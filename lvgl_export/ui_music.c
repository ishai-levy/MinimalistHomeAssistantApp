/* ui_music.c — Music tab
 * Left panel (580px): 500×500 album art, title/artist, seekbar, transport,
 *                     player dropdown, volume bar
 * Right panel (700px): scrollable Up-Next queue / Playlists toggle
 */
#include "ui.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Widget handles ─────────────────────────────────────────────────── */
static lv_obj_t *s_art;
static lv_obj_t *s_title;
static lv_obj_t *s_artist;
static lv_obj_t *s_seek_ind;
static lv_obj_t *s_pos_lbl;
static lv_obj_t *s_dur_lbl;
static lv_obj_t *s_play_lbl;
static lv_obj_t *s_shuf_btn;
static lv_obj_t *s_fav_btn;
static lv_obj_t *s_vol_ind;
static lv_obj_t *s_vol_lbl;
static lv_obj_t *s_player_dd;
static lv_obj_t *s_queue_tab;
static lv_obj_t *s_pl_tab;
static lv_obj_t *s_list;          /* scrollable list container */

/* ── State ──────────────────────────────────────────────────────────── */
static bool   s_playing   = false;
static bool   s_shuffle   = false;
static bool   s_favorited = false;
static int    s_pos_ms    = 0;
static int    s_dur_ms    = 0;
static int    s_volume    = 70;
static bool   s_view_pl   = false;   /* false = queue, true = playlists */
static char   s_track_uri[256];

#define MAX_Q  24
#define MAX_PL 32
static ui_queue_item_t s_queue[MAX_Q];
static int             s_queue_n = 0;
static ui_playlist_t   s_pl[MAX_PL];
static int             s_pl_n    = 0;

/* user-data structs for playlist play events (static pool, safe pointer lifetime) */
typedef struct { char uri[256]; } _pl_ud_t;
static _pl_ud_t _pl_ud[MAX_PL];

/* ── Seek timer ─────────────────────────────────────────────────────── */
static lv_timer_t *s_seek_timer;

static void _seek_tick(lv_timer_t *t) {
    (void)t;
    if (!s_playing || s_dur_ms <= 0) return;
    s_pos_ms += 1000;
    if (s_pos_ms >= s_dur_ms) s_pos_ms = s_dur_ms;

    int pct = (s_dur_ms > 0) ? (s_pos_ms * 100 / s_dur_ms) : 0;
    lv_obj_set_width(s_seek_ind, LV_PCT(pct));

    int ps = s_pos_ms / 1000, ds = s_dur_ms / 1000;
    lv_label_set_text_fmt(s_pos_lbl, "%d:%02d", ps / 60, ps % 60);
    lv_label_set_text_fmt(s_dur_lbl, "%d:%02d", ds / 60, ds % 60);
}

/* ── Transport callbacks ────────────────────────────────────────────── */
static void _play_cb(lv_event_t *e) {
    (void)e;
    s_playing = !s_playing;
    lv_label_set_text(s_play_lbl, s_playing ? "II" : ">");
    /* TODO: mqtt publish dashboard/cmd/media_play_pause */
}
static void _prev_cb(lv_event_t *e) {
    (void)e;
    /* TODO: mqtt publish dashboard/cmd/media_previous */
}
static void _next_cb(lv_event_t *e) {
    (void)e;
    /* TODO: mqtt publish dashboard/cmd/media_next */
}
static void _shuf_cb(lv_event_t *e) {
    (void)e;
    s_shuffle = !s_shuffle;
    lv_obj_set_style_border_color(s_shuf_btn,
        s_shuffle ? UI_CLR_ACCENT : UI_CLR_BORDER, 0);
    lv_obj_set_style_bg_color(s_shuf_btn,
        s_shuffle ? UI_CLR_ACCENT_BG : UI_CLR_SURF3, 0);
    /* TODO: mqtt publish dashboard/cmd/shuffle */
}
static void _fav_cb(lv_event_t *e) {
    (void)e;
    s_favorited = !s_favorited;
    lv_obj_set_style_border_color(s_fav_btn,
        s_favorited ? UI_CLR_WARM : UI_CLR_BORDER, 0);
    lv_obj_set_style_bg_color(s_fav_btn,
        s_favorited ? UI_CLR_WARM_BG : UI_CLR_SURF3, 0);
    /* TODO: mqtt publish dashboard/cmd/favorite_add  (or favorite_remove) with s_track_uri */
}
static void _radio_cb(lv_event_t *e) {
    (void)e;
    /* TODO: mqtt publish dashboard/cmd/start_radio */
}

/* ── Volume bar tap ─────────────────────────────────────────────────── */
static void _vol_click_cb(lv_event_t *e) {
    lv_obj_t *bar = lv_event_get_target(e);
    lv_point_t pt;
    lv_indev_get_point(lv_indev_get_act(), &pt);
    lv_area_t area;
    lv_obj_get_coords(bar, &area);
    int w = area.x2 - area.x1;
    if (w > 0) {
        s_volume = (pt.x - area.x1) * 100 / w;
        if (s_volume < 0)   s_volume = 0;
        if (s_volume > 100) s_volume = 100;
        lv_obj_set_width(s_vol_ind, LV_PCT(s_volume));
        lv_label_set_text_fmt(s_vol_lbl, "%d", s_volume);
        /* TODO: mqtt publish dashboard/cmd/media_volume with s_volume */
    }
}

/* ── Queue / Playlist toggle ────────────────────────────────────────── */
static void _rebuild_list(void);

static void _view_queue_cb(lv_event_t *e) {
    (void)e;
    if (!s_view_pl) return;
    s_view_pl = false;
    lv_obj_set_style_bg_color(s_queue_tab, UI_CLR_ACCENT, 0);
    lv_obj_set_style_bg_opa(s_queue_tab, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(s_pl_tab, LV_OPA_TRANSP, 0);
    _rebuild_list();
}
static void _view_pl_cb(lv_event_t *e) {
    (void)e;
    if (s_view_pl) return;
    s_view_pl = true;
    lv_obj_set_style_bg_color(s_pl_tab, UI_CLR_ACCENT, 0);
    lv_obj_set_style_bg_opa(s_pl_tab, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(s_queue_tab, LV_OPA_TRANSP, 0);
    _rebuild_list();
}

static void _pl_play_cb(lv_event_t *e) {
    _pl_ud_t *d = (_pl_ud_t *)lv_event_get_user_data(e);
    /* TODO: mqtt publish dashboard/cmd/play_playlist with d->uri */
    (void)d;
}

static void _rebuild_list(void) {
    lv_obj_clean(s_list);

    if (!s_view_pl) {
        /* ── Up Next ─────────────────────────────────────────────────── */
        for (int i = 0; i < s_queue_n; i++) {
            lv_obj_t *row = lv_obj_create(s_list);
            lv_obj_set_width(row, LV_PCT(100));
            lv_obj_set_height(row, LV_SIZE_CONTENT);
            ui_style_ghost(row);
            lv_obj_set_style_pad_all(row, 12, 0);
            lv_obj_set_style_pad_hor(row, 14, 0);
            lv_obj_set_layout(row, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                                   LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
            lv_obj_set_style_pad_column(row, 14, 0);

            lv_obj_t *num = lv_label_create(row);
            lv_obj_set_style_text_font(num, UI_FONT_MONO_11, 0);
            lv_obj_set_style_text_color(num, UI_CLR_TEXT3, 0);
            lv_obj_set_width(num, 22);
            lv_label_set_text_fmt(num, "%d", i + 1);

            lv_obj_t *info = lv_obj_create(row);
            lv_obj_set_flex_grow(info, 1);
            lv_obj_set_height(info, LV_SIZE_CONTENT);
            ui_style_ghost(info);
            lv_obj_set_layout(info, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_pad_row(info, 3, 0);

            lv_obj_t *tl = lv_label_create(info);
            lv_obj_set_style_text_font(tl, UI_FONT_UI_14, 0);
            lv_obj_set_style_text_color(tl, UI_CLR_TEXT, 0);
            lv_obj_set_width(tl, LV_PCT(100));
            lv_label_set_long_mode(tl, LV_LABEL_LONG_DOT);
            lv_label_set_text(tl, s_queue[i].title);

            lv_obj_t *al = lv_label_create(info);
            lv_obj_set_style_text_font(al, UI_FONT_UI_12, 0);
            lv_obj_set_style_text_color(al, UI_CLR_TEXT3, 0);
            lv_obj_set_width(al, LV_PCT(100));
            lv_label_set_long_mode(al, LV_LABEL_LONG_DOT);
            lv_label_set_text(al, s_queue[i].artist);
        }
    } else {
        /* ── Playlists ───────────────────────────────────────────────── */
        for (int i = 0; i < s_pl_n; i++) {
            strncpy(_pl_ud[i].uri, s_pl[i].uri, sizeof(_pl_ud[i].uri) - 1);

            lv_obj_t *row = lv_obj_create(s_list);
            lv_obj_set_width(row, LV_PCT(100));
            lv_obj_set_height(row, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(row, UI_CLR_SURF2, 0);
            lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(row, UI_CLR_BORDER, 0);
            lv_obj_set_style_border_width(row, 1, 0);
            lv_obj_set_style_radius(row, 10, 0);
            lv_obj_set_style_pad_all(row, 14, 0);
            lv_obj_set_style_pad_hor(row, 16, 0);
            lv_obj_set_layout(row, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                                   LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
            lv_obj_set_style_pad_column(row, 14, 0);
            lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_event_cb(row, _pl_play_cb, LV_EVENT_CLICKED, &_pl_ud[i]);

            /* Art placeholder box */
            lv_obj_t *art = lv_obj_create(row);
            lv_obj_set_size(art, 40, 40);
            lv_obj_set_style_bg_color(art, UI_CLR_SURF3, 0);
            lv_obj_set_style_bg_opa(art, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(art, UI_CLR_BORDER, 0);
            lv_obj_set_style_border_width(art, 1, 0);
            lv_obj_set_style_radius(art, 8, 0);
            lv_obj_set_layout(art, LV_LAYOUT_FLEX);
            lv_obj_set_flex_align(art, LV_FLEX_ALIGN_CENTER,
                                   LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_clear_flag(art, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_t *art_lbl = lv_label_create(art);
            lv_obj_set_style_text_font(art_lbl, UI_FONT_UI_12, 0);
            lv_obj_set_style_text_color(art_lbl, UI_CLR_TEXT3, 0);
            lv_label_set_text(art_lbl, "M");

            lv_obj_t *info = lv_obj_create(row);
            lv_obj_set_flex_grow(info, 1);
            lv_obj_set_height(info, LV_SIZE_CONTENT);
            ui_style_ghost(info);
            lv_obj_set_layout(info, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_pad_row(info, 4, 0);

            lv_obj_t *nl = lv_label_create(info);
            lv_obj_set_style_text_font(nl, UI_FONT_UI_14, 0);
            lv_obj_set_style_text_color(nl, UI_CLR_TEXT, 0);
            lv_obj_set_width(nl, LV_PCT(100));
            lv_label_set_long_mode(nl, LV_LABEL_LONG_DOT);
            lv_label_set_text(nl, s_pl[i].name);

            lv_obj_t *ul = lv_label_create(info);
            lv_obj_set_style_text_font(ul, UI_FONT_MONO_10, 0);
            lv_obj_set_style_text_color(ul, UI_CLR_TEXT3, 0);
            lv_obj_set_width(ul, LV_PCT(100));
            lv_label_set_long_mode(ul, LV_LABEL_LONG_DOT);
            lv_label_set_text(ul, s_pl[i].uri);
        }
    }
}

/* ── Transport button factory ───────────────────────────────────────── */
static lv_obj_t *_transport_btn(lv_obj_t *parent, const char *label,
                                  bool big, lv_event_cb_t cb) {
    int sz = big ? 60 : 48;
    lv_obj_t *b = lv_obj_create(parent);
    lv_obj_set_size(b, sz, sz);
    lv_obj_set_style_radius(b, sz / 2, 0);
    lv_obj_set_style_bg_color(b, big ? UI_CLR_ACCENT : UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(b, big ? 0 : 1, 0);
    lv_obj_set_layout(b, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_style_text_font(l, UI_FONT_UI_16, 0);
    lv_obj_set_style_text_color(l, big ? UI_CLR_BG : UI_CLR_TEXT2, 0);
    lv_label_set_text(l, label);
    return l;   /* return label so callers can swap icon text */
}

/* ── Icon-style circle button (fav / shuffle) ───────────────────────── */
static lv_obj_t *_icon_btn(lv_obj_t *parent, const char *text,
                             lv_event_cb_t cb) {
    lv_obj_t *b = lv_obj_create(parent);
    lv_obj_set_size(b, 48, 48);
    lv_obj_set_style_radius(b, 24, 0);
    lv_obj_set_style_bg_color(b, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_layout(b, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_style_text_font(l, UI_FONT_UI_14, 0);
    lv_obj_set_style_text_color(l, UI_CLR_TEXT2, 0);
    lv_label_set_text(l, text);
    return b;
}

/* ══════════════════════════════════════════════════════════════════════
 * ui_music_build
 * ══════════════════════════════════════════════════════════════════════ */
void ui_music_build(lv_obj_t *parent) {

    /* ── Left panel: 580px wide ──────────────────────────────────────── */
    lv_obj_t *left = lv_obj_create(parent);
    lv_obj_set_size(left, 580, LV_PCT(100));
    lv_obj_set_pos(left, 0, 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(left, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(left, 1, 0);
    lv_obj_set_style_border_side(left, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_pad_all(left, 16, 0);
    lv_obj_set_style_pad_hor(left, 40, 0);
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(left, 0, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    /* Album art — 500×500 */
    s_art = lv_obj_create(left);
    lv_obj_set_size(s_art, 500, 500);
    lv_obj_set_style_bg_color(s_art, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(s_art, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_art, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(s_art, 1, 0);
    lv_obj_set_style_radius(s_art, 16, 0);
    lv_obj_set_layout(s_art, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(s_art, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_art, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *art_ph = lv_label_create(s_art);
    lv_obj_set_style_text_font(art_ph, UI_FONT_UI_14, 0);
    lv_obj_set_style_text_color(art_ph, UI_CLR_TEXT3, 0);
    lv_label_set_text(art_ph, "No Art");
    /* TODO: use lv_img after fetching from np->art_url via HTTP → PSRAM buffer */

    /* Title + artist */
    lv_obj_t *meta = lv_obj_create(left);
    lv_obj_set_size(meta, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(meta);
    lv_obj_set_layout(meta, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(meta, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(meta, 3, 0);
    lv_obj_set_style_margin_top(meta, 10, 0);

    s_title = lv_label_create(meta);
    lv_obj_set_style_text_font(s_title, UI_FONT_UI_16, 0);
    lv_obj_set_style_text_color(s_title, UI_CLR_TEXT, 0);
    lv_obj_set_width(s_title, LV_PCT(100));
    lv_label_set_long_mode(s_title, LV_LABEL_LONG_DOT);
    lv_label_set_text(s_title, "Nothing playing");

    s_artist = lv_label_create(meta);
    lv_obj_set_style_text_font(s_artist, UI_FONT_UI_13, 0);
    lv_obj_set_style_text_color(s_artist, UI_CLR_TEXT2, 0);
    lv_obj_set_width(s_artist, LV_PCT(100));
    lv_label_set_long_mode(s_artist, LV_LABEL_LONG_DOT);
    lv_label_set_text(s_artist, "");

    /* Seekbar */
    lv_obj_t *seek_wrap = lv_obj_create(left);
    lv_obj_set_size(seek_wrap, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(seek_wrap);
    lv_obj_set_layout(seek_wrap, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(seek_wrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(seek_wrap, 6, 0);
    lv_obj_set_style_margin_top(seek_wrap, 12, 0);

    lv_obj_t *seek_track = lv_obj_create(seek_wrap);
    lv_obj_set_size(seek_track, LV_PCT(100), 4);
    lv_obj_set_style_bg_color(seek_track, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(seek_track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(seek_track, 0, 0);
    lv_obj_set_style_radius(seek_track, 2, 0);
    lv_obj_add_flag(seek_track, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(seek_track, LV_OBJ_FLAG_SCROLLABLE);
    /* TODO: on click, compute pct and publish dashboard/cmd/media_seek */

    s_seek_ind = lv_obj_create(seek_track);
    lv_obj_set_size(s_seek_ind, LV_PCT(0), LV_PCT(100));
    lv_obj_set_pos(s_seek_ind, 0, 0);
    lv_obj_set_style_bg_color(s_seek_ind, UI_CLR_ACCENT, 0);
    lv_obj_set_style_bg_opa(s_seek_ind, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_seek_ind, 0, 0);
    lv_obj_set_style_radius(s_seek_ind, 2, 0);
    lv_obj_clear_flag(s_seek_ind, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *time_row = lv_obj_create(seek_wrap);
    lv_obj_set_size(time_row, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(time_row);
    lv_obj_set_layout(time_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    s_pos_lbl = lv_label_create(time_row);
    lv_obj_set_style_text_font(s_pos_lbl, UI_FONT_MONO_11, 0);
    lv_obj_set_style_text_color(s_pos_lbl, UI_CLR_TEXT3, 0);
    lv_label_set_text(s_pos_lbl, "0:00");

    s_dur_lbl = lv_label_create(time_row);
    lv_obj_set_style_text_font(s_dur_lbl, UI_FONT_MONO_11, 0);
    lv_obj_set_style_text_color(s_dur_lbl, UI_CLR_TEXT3, 0);
    lv_label_set_text(s_dur_lbl, "0:00");

    /* Transport row */
    lv_obj_t *transport = lv_obj_create(left);
    lv_obj_set_size(transport, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(transport);
    lv_obj_set_layout(transport, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(transport, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(transport, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(transport, 10, 0);
    lv_obj_set_style_margin_top(transport, 10, 0);

    s_fav_btn = _icon_btn(transport, "Fav", _fav_cb);
    _transport_btn(transport, "|<", false, _prev_cb);
    s_play_lbl = _transport_btn(transport, ">", true, _play_cb);
    _transport_btn(transport, ">|", false, _next_cb);
    s_shuf_btn = _icon_btn(transport, "Shuf", _shuf_cb);
    _icon_btn(transport, "Radio", _radio_cb);

    /* Player dropdown + Volume */
    lv_obj_t *pv = lv_obj_create(left);
    lv_obj_set_size(pv, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(pv);
    lv_obj_set_layout(pv, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pv, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pv, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(pv, 16, 0);
    lv_obj_set_style_margin_top(pv, 12, 0);

    s_player_dd = lv_dropdown_create(pv);
    lv_obj_set_width(s_player_dd, LV_SIZE_CONTENT);
    lv_dropdown_set_options(s_player_dd, "Home group\nKitchen\nBedroom");
    lv_obj_set_style_bg_color(s_player_dd, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(s_player_dd, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_player_dd, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(s_player_dd, 1, 0);
    lv_obj_set_style_text_font(s_player_dd, UI_FONT_UI_12, 0);
    lv_obj_set_style_text_color(s_player_dd, UI_CLR_TEXT2, 0);
    /* TODO: on LV_EVENT_VALUE_CHANGED, publish dashboard/cmd/select_player */

    /* Volume bar */
    lv_obj_t *vol_wrap = lv_obj_create(pv);
    lv_obj_set_flex_grow(vol_wrap, 1);
    lv_obj_set_height(vol_wrap, LV_SIZE_CONTENT);
    ui_style_ghost(vol_wrap);
    lv_obj_set_layout(vol_wrap, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(vol_wrap, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vol_wrap, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(vol_wrap, 8, 0);

    lv_obj_t *vol_tag = lv_label_create(vol_wrap);
    lv_obj_set_style_text_font(vol_tag, UI_FONT_MONO_10, 0);
    lv_obj_set_style_text_color(vol_tag, UI_CLR_TEXT3, 0);
    lv_label_set_text(vol_tag, "VOL");

    lv_obj_t *vol_track = lv_obj_create(vol_wrap);
    lv_obj_set_flex_grow(vol_track, 1);
    lv_obj_set_height(vol_track, 4);
    lv_obj_set_style_bg_color(vol_track, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(vol_track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vol_track, 0, 0);
    lv_obj_set_style_radius(vol_track, 2, 0);
    lv_obj_add_flag(vol_track, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(vol_track, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(vol_track, _vol_click_cb, LV_EVENT_CLICKED, NULL);

    s_vol_ind = lv_obj_create(vol_track);
    lv_obj_set_size(s_vol_ind, LV_PCT(s_volume), LV_PCT(100));
    lv_obj_set_pos(s_vol_ind, 0, 0);
    lv_obj_set_style_bg_color(s_vol_ind, UI_CLR_ACCENT, 0);
    lv_obj_set_style_bg_opa(s_vol_ind, UI_OPA_VOL_IND, 0);
    lv_obj_set_style_border_width(s_vol_ind, 0, 0);
    lv_obj_set_style_radius(s_vol_ind, 2, 0);
    lv_obj_clear_flag(s_vol_ind, LV_OBJ_FLAG_SCROLLABLE);

    s_vol_lbl = lv_label_create(vol_wrap);
    lv_obj_set_style_text_font(s_vol_lbl, UI_FONT_MONO_11, 0);
    lv_obj_set_style_text_color(s_vol_lbl, UI_CLR_TEXT2, 0);
    lv_obj_set_width(s_vol_lbl, 28);
    lv_label_set_text_fmt(s_vol_lbl, "%d", s_volume);

    /* ── Right panel: 700px ──────────────────────────────────────────── */
    lv_obj_t *right = lv_obj_create(parent);
    lv_obj_set_size(right, 700, LV_PCT(100));
    lv_obj_set_pos(right, 580, 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right, 0, 0);
    lv_obj_set_style_pad_all(right, 24, 0);
    lv_obj_set_style_pad_hor(right, 32, 0);
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(right, 0, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    /* Toggle pill: Up Next / Playlists */
    lv_obj_t *pill = lv_obj_create(right);
    lv_obj_set_size(pill, LV_SIZE_CONTENT, 34);
    lv_obj_set_style_bg_color(pill, UI_CLR_SURF2, 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(pill, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(pill, 1, 0);
    lv_obj_set_style_radius(pill, 10, 0);
    lv_obj_set_style_pad_all(pill, 3, 0);
    lv_obj_set_style_pad_column(pill, 3, 0);
    lv_obj_set_style_margin_bottom(pill, 18, 0);
    lv_obj_set_layout(pill, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pill, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

    typedef struct { const char *label; lv_event_cb_t cb; lv_obj_t **ref; bool active; } _tt;
    _tt tabs[2] = {
        {"Up Next",   _view_queue_cb, &s_queue_tab, true},
        {"Playlists", _view_pl_cb,    &s_pl_tab,    false},
    };
    for (int i = 0; i < 2; i++) {
        lv_obj_t *b = lv_obj_create(pill);
        lv_obj_set_height(b, 28);
        lv_obj_set_width(b, LV_SIZE_CONTENT);
        lv_obj_set_style_radius(b, 7, 0);
        lv_obj_set_style_bg_color(b, UI_CLR_ACCENT, 0);
        lv_obj_set_style_bg_opa(b, tabs[i].active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_set_style_pad_hor(b, 20, 0);
        lv_obj_set_style_pad_ver(b, 0, 0);
        lv_obj_set_layout(b, LV_LAYOUT_FLEX);
        lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER,
                               LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(b, tabs[i].cb, LV_EVENT_CLICKED, NULL);
        *tabs[i].ref = b;

        lv_obj_t *l = lv_label_create(b);
        lv_obj_set_style_text_font(l, UI_FONT_UI_12, 0);
        lv_obj_set_style_text_color(l,
            tabs[i].active ? UI_CLR_BG : UI_CLR_TEXT2, 0);
        lv_label_set_text(l, tabs[i].label);
    }

    /* Scrollable list */
    s_list = lv_obj_create(right);
    lv_obj_set_flex_grow(s_list, 1);
    lv_obj_set_width(s_list, LV_PCT(100));
    ui_style_ghost(s_list);
    lv_obj_set_style_pad_bottom(s_list, 20, 0);
    ui_style_scroll_col(s_list);
    lv_obj_set_style_pad_row(s_list, 4, 0);

    /* Seed with empty queue */
    _rebuild_list();

    /* Start seek timer */
    s_seek_timer = lv_timer_create(_seek_tick, 1000, NULL);
}

/* ══════════════════════════════════════════════════════════════════════
 * State update functions
 * ══════════════════════════════════════════════════════════════════════ */
void ui_update_now_playing(const ui_now_playing_t *np) {
    s_playing  = np->playing;
    s_pos_ms   = np->position_ms;
    s_dur_ms   = np->duration_ms;
    s_volume   = np->volume;
    s_shuffle  = np->shuffle;
    strncpy(s_track_uri, np->uri, sizeof(s_track_uri) - 1);

    lv_label_set_text(s_title,  np->title[0]  ? np->title  : "Nothing playing");
    lv_label_set_text(s_artist, np->artist[0] ? np->artist : "");
    lv_label_set_text(s_play_lbl, np->playing ? "II" : ">");

    int pct = (np->duration_ms > 0)
        ? np->position_ms * 100 / np->duration_ms : 0;
    lv_obj_set_width(s_seek_ind, LV_PCT(pct));

    int ps = s_pos_ms / 1000, ds = s_dur_ms / 1000;
    lv_label_set_text_fmt(s_pos_lbl, "%d:%02d", ps / 60, ps % 60);
    lv_label_set_text_fmt(s_dur_lbl, "%d:%02d", ds / 60, ds % 60);

    lv_obj_set_width(s_vol_ind, LV_PCT(np->volume));
    lv_label_set_text_fmt(s_vol_lbl, "%d", np->volume);

    /* Shuffle button highlight */
    lv_obj_set_style_border_color(s_shuf_btn,
        np->shuffle ? UI_CLR_ACCENT : UI_CLR_BORDER, 0);
    lv_obj_set_style_bg_color(s_shuf_btn,
        np->shuffle ? UI_CLR_ACCENT_BG : UI_CLR_SURF3, 0);

    /* TODO: async-fetch np->art_url and load into s_art via lv_img_set_src */
}

void ui_update_queue_next(const ui_queue_item_t *items, int count) {
    int n = count < MAX_Q ? count : MAX_Q;
    memcpy(s_queue, items, n * sizeof(ui_queue_item_t));
    s_queue_n = n;
    if (!s_view_pl) _rebuild_list();
}

void ui_update_shuffle(bool on) {
    s_shuffle = on;
    lv_obj_set_style_border_color(s_shuf_btn, on ? UI_CLR_ACCENT : UI_CLR_BORDER, 0);
    lv_obj_set_style_bg_color(s_shuf_btn, on ? UI_CLR_ACCENT_BG : UI_CLR_SURF3, 0);
}

void ui_update_players(const ui_player_t *players, int count) {
    /* Rebuild dropdown options from live player list */
    if (count <= 0) return;
    char buf[512] = {0};
    for (int i = 0; i < count && (int)strlen(buf) < 480; i++) {
        if (i > 0) strcat(buf, "\n");
        strncat(buf, players[i].name, 63);
    }
    lv_dropdown_set_options(s_player_dd, buf);
}

void ui_update_playlists(const ui_playlist_t *playlists, int count) {
    int n = count < MAX_PL ? count : MAX_PL;
    memcpy(s_pl, playlists, n * sizeof(ui_playlist_t));
    s_pl_n = n;
    if (s_view_pl) _rebuild_list();
}
