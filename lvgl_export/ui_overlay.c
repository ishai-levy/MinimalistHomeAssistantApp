/* ui_overlay.c — Modal overlays
 *
 *  ui_overlay_show_alarm_modal()  — HH:MM spinners, player dropdown,
 *                                   confirm button, upcoming alarms list
 *  ui_update_ringing()            — full-screen ringing popup with STOP
 */
#include "ui.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ════════════════════════════════════════════════════════════════════════
 * Shared alarm state
 * ════════════════════════════════════════════════════════════════════════ */
#define MAX_ALARMS 8
static ui_schedule_entry_t s_alarms[MAX_ALARMS];
static int                  s_alarm_n = 0;

/* Called by ui_home.c's ui_update_alarms() — forward so Home can share data */
void _overlay_store_alarms(const ui_schedule_entry_t *e, int n) {
    int c = n < MAX_ALARMS ? n : MAX_ALARMS;
    memcpy(s_alarms, e, c * sizeof(ui_schedule_entry_t));
    s_alarm_n = c;
}

/* ════════════════════════════════════════════════════════════════════════
 * Alarm modal
 * ════════════════════════════════════════════════════════════════════════ */
static lv_obj_t *s_modal_bg    = NULL;
static lv_obj_t *s_alarm_list  = NULL;   /* upcoming alarms container */
static int        s_hh = 7, s_mm = 0;
static lv_obj_t  *s_hh_lbl, *s_mm_lbl;
static lv_obj_t  *s_confirm_lbl;
static lv_obj_t  *s_player_dd_alarm;

/* ── Spinner callbacks ──────────────────────────────────────────────── */
static void _hh_inc(lv_event_t *e) { (void)e; s_hh=(s_hh+1)%24;
    lv_label_set_text_fmt(s_hh_lbl,"%02d",s_hh);
    lv_label_set_text_fmt(s_confirm_lbl,"Set for %02d:%02d",s_hh,s_mm); }
static void _hh_dec(lv_event_t *e) { (void)e; s_hh=(s_hh+23)%24;
    lv_label_set_text_fmt(s_hh_lbl,"%02d",s_hh);
    lv_label_set_text_fmt(s_confirm_lbl,"Set for %02d:%02d",s_hh,s_mm); }
static void _mm_inc(lv_event_t *e) { (void)e; s_mm=(s_mm+1)%60;
    lv_label_set_text_fmt(s_mm_lbl,"%02d",s_mm);
    lv_label_set_text_fmt(s_confirm_lbl,"Set for %02d:%02d",s_hh,s_mm); }
static void _mm_dec(lv_event_t *e) { (void)e; s_mm=(s_mm+59)%60;
    lv_label_set_text_fmt(s_mm_lbl,"%02d",s_mm);
    lv_label_set_text_fmt(s_confirm_lbl,"Set for %02d:%02d",s_hh,s_mm); }

/* ── Close ──────────────────────────────────────────────────────────── */
static void _modal_close(lv_event_t *e) {
    (void)e;
    if (s_modal_bg) { lv_obj_del(s_modal_bg); s_modal_bg = NULL; }
}

/* Block tap propagation to the darkened backdrop */
static void _card_click_stop(lv_event_t *e) { (void)e; /* consume */ }

/* ── Confirm ────────────────────────────────────────────────────────── */
static void _confirm(lv_event_t *e) {
    (void)e;
    /* Compute next occurrence of HH:MM */
    time_t now = time(NULL);
    struct tm tm_next = *localtime(&now);
    tm_next.tm_hour = s_hh;
    tm_next.tm_min  = s_mm;
    tm_next.tm_sec  = 0;
    time_t trigger  = mktime(&tm_next);
    if (trigger <= now) trigger += 86400;

    /* Read selected player */
    char player_name[64];
    lv_dropdown_get_selected_str(s_player_dd_alarm, player_name, sizeof(player_name));

    /* TODO: mqtt publish:
     *   topic  = "dashboard/cmd/set_alarm"
     *   payload = {"time": "HH:MM", "player_id": player_name}
     */
    (void)trigger;

    _modal_close(NULL);
}

/* ── Alarm entry cancel ─────────────────────────────────────────────── */
static void _cancel_alarm(lv_event_t *e) {
    char *id = (char *)lv_event_get_user_data(e);
    /* TODO: mqtt publish dashboard/cmd/cancel_alarm with id */
    (void)id;
}

/* ── Spinner column factory ─────────────────────────────────────────── */
static lv_obj_t *_spinner_col(lv_obj_t *parent, int val,
                                lv_obj_t **out_lbl,
                                lv_event_cb_t inc_cb,
                                lv_event_cb_t dec_cb) {
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, 90, LV_SIZE_CONTENT);
    ui_style_ghost(col);
    lv_obj_set_layout(col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col, 10, 0);

    /* ▲ button */
    lv_obj_t *up = lv_obj_create(col);
    lv_obj_set_size(up, 46, 46);
    lv_obj_set_style_bg_color(up, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(up, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(up, UI_CLR_BORDER_BR, 0);
    lv_obj_set_style_border_width(up, 1, 0);
    lv_obj_set_style_radius(up, 10, 0);
    lv_obj_set_layout(up, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(up, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(up, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(up, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(up, inc_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *up_l = lv_label_create(up);
    lv_obj_set_style_text_font(up_l, UI_FONT_UI_16, 0);
    lv_obj_set_style_text_color(up_l, UI_CLR_TEXT2, 0);
    lv_label_set_text(up_l, "^");

    /* value label */
    *out_lbl = lv_label_create(col);
    lv_obj_set_style_text_font(*out_lbl, UI_FONT_MONO_48, 0);
    lv_obj_set_style_text_color(*out_lbl, UI_CLR_TEXT, 0);
    lv_obj_set_width(*out_lbl, 90);
    lv_obj_set_style_text_align(*out_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(*out_lbl, "%02d", val);

    /* ▼ button */
    lv_obj_t *dn = lv_obj_create(col);
    lv_obj_set_size(dn, 46, 46);
    lv_obj_set_style_bg_color(dn, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(dn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dn, UI_CLR_BORDER_BR, 0);
    lv_obj_set_style_border_width(dn, 1, 0);
    lv_obj_set_style_radius(dn, 10, 0);
    lv_obj_set_layout(dn, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(dn, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(dn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(dn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(dn, dec_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *dn_l = lv_label_create(dn);
    lv_obj_set_style_text_font(dn_l, UI_FONT_UI_16, 0);
    lv_obj_set_style_text_color(dn_l, UI_CLR_TEXT2, 0);
    lv_label_set_text(dn_l, "v");

    return col;
}

/* ── Rebuild upcoming alarm list inside an open modal ───────────────── */
static void _rebuild_alarm_list(void) {
    if (!s_alarm_list) return;
    lv_obj_clean(s_alarm_list);

    for (int i = 0; i < s_alarm_n; i++) {
        lv_obj_t *row = lv_obj_create(s_alarm_list);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(row, UI_CLR_SURF2, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, UI_CLR_BORDER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 10, 0);
        lv_obj_set_style_pad_all(row, 12, 0);
        lv_obj_set_style_pad_hor(row, 14, 0);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                               LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        /* Time + player */
        lv_obj_t *info = lv_obj_create(row);
        ui_style_ghost(info);
        lv_obj_set_size(info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_layout(info, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(info, 3, 0);

        time_t t = (time_t)s_alarms[i].trigger_at;
        struct tm *tm = localtime(&t);
        int h12 = tm->tm_hour % 12; if (!h12) h12 = 12;
        char ts[20];
        snprintf(ts, sizeof(ts), "%02d:%02d %s",
            h12, tm->tm_min, tm->tm_hour >= 12 ? "PM" : "AM");

        lv_obj_t *tl = lv_label_create(info);
        lv_obj_set_style_text_font(tl, UI_FONT_MONO_18, 0);
        lv_obj_set_style_text_color(tl, UI_CLR_TEXT, 0);
        lv_label_set_text(tl, ts);

        lv_obj_t *pl = lv_label_create(info);
        lv_obj_set_style_text_font(pl, UI_FONT_UI_11, 0);
        lv_obj_set_style_text_color(pl, UI_CLR_TEXT3, 0);
        lv_label_set_text(pl, s_alarms[i].player_id);

        /* ✕ cancel button */
        lv_obj_t *xbtn = lv_obj_create(row);
        lv_obj_set_size(xbtn, 32, 32);
        lv_obj_set_style_bg_color(xbtn, UI_CLR_WARM_BG, 0);
        lv_obj_set_style_bg_opa(xbtn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(xbtn, UI_CLR_WARM, 0);
        lv_obj_set_style_border_width(xbtn, 1, 0);
        lv_obj_set_style_radius(xbtn, 7, 0);
        lv_obj_set_layout(xbtn, LV_LAYOUT_FLEX);
        lv_obj_set_flex_align(xbtn, LV_FLEX_ALIGN_CENTER,
                               LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_add_flag(xbtn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(xbtn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(xbtn, _cancel_alarm, LV_EVENT_CLICKED,
                             (void *)s_alarms[i].id);
        lv_obj_t *xl = lv_label_create(xbtn);
        lv_obj_set_style_text_font(xl, UI_FONT_UI_14, 0);
        lv_obj_set_style_text_color(xl, UI_CLR_WARM, 0);
        lv_label_set_text(xl, "X");
    }
}

/* ════════════════════════════════════════════════════════════════════════
 * ui_overlay_show_alarm_modal
 * ════════════════════════════════════════════════════════════════════════ */
void ui_overlay_show_alarm_modal(void) {
    if (s_modal_bg) return;   /* already open */

    /* Darkened full-screen backdrop — rendered above all other objects */
    s_modal_bg = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_modal_bg, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_modal_bg, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_modal_bg, UI_OPA_MODAL, 0);
    lv_obj_set_style_border_width(s_modal_bg, 0, 0);
    lv_obj_set_style_pad_all(s_modal_bg, 0, 0);
    lv_obj_set_layout(s_modal_bg, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(s_modal_bg, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(s_modal_bg, _modal_close, LV_EVENT_CLICKED, NULL);
    lv_obj_clear_flag(s_modal_bg, LV_OBJ_FLAG_SCROLLABLE);

    /* Modal card */
    lv_obj_t *card = lv_obj_create(s_modal_bg);
    lv_obj_set_size(card, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(card, 700, 0);
    lv_obj_set_style_bg_color(card, UI_CLR_SURF, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, UI_CLR_BORDER_BR, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 22, 0);
    lv_obj_set_style_pad_all(card, 36, 0);
    lv_obj_set_style_pad_hor(card, 42, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(card, 22, 0);
    lv_obj_add_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(card, LV_DIR_VER);
    /* Stop taps from reaching the backdrop */
    lv_obj_add_event_cb(card, _card_click_stop, LV_EVENT_CLICKED, NULL);

    /* Header */
    lv_obj_t *hdr = lv_obj_create(card);
    lv_obj_set_size(hdr, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(hdr);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    lv_obj_t *title = lv_label_create(hdr);
    lv_obj_set_style_text_font(title, UI_FONT_UI_16, 0);
    lv_obj_set_style_text_color(title, UI_CLR_TEXT, 0);
    lv_label_set_text(title, "Set Alarm");

    lv_obj_t *xbtn = lv_obj_create(hdr);
    lv_obj_set_size(xbtn, 34, 34);
    lv_obj_set_style_bg_color(xbtn, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(xbtn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(xbtn, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(xbtn, 1, 0);
    lv_obj_set_style_radius(xbtn, 8, 0);
    lv_obj_set_layout(xbtn, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(xbtn, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(xbtn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(xbtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(xbtn, _modal_close, LV_EVENT_CLICKED, NULL);
    lv_obj_t *xl = lv_label_create(xbtn);
    lv_obj_set_style_text_font(xl, UI_FONT_UI_14, 0);
    lv_obj_set_style_text_color(xl, UI_CLR_TEXT2, 0);
    lv_label_set_text(xl, "X");

    /* Time picker row: HH : MM */
    lv_obj_t *picker = lv_obj_create(card);
    lv_obj_set_size(picker, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(picker);
    lv_obj_set_layout(picker, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(picker, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(picker, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(picker, 14, 0);

    _spinner_col(picker, s_hh, &s_hh_lbl, _hh_inc, _hh_dec);

    lv_obj_t *colon = lv_label_create(picker);
    lv_obj_set_style_text_font(colon, UI_FONT_MONO_48, 0);
    lv_obj_set_style_text_color(colon, UI_CLR_ACCENT, 0);
    lv_obj_set_style_margin_bottom(colon, 8, 0);
    lv_label_set_text(colon, ":");

    _spinner_col(picker, s_mm, &s_mm_lbl, _mm_inc, _mm_dec);

    /* Player dropdown */
    s_player_dd_alarm = lv_dropdown_create(card);
    lv_obj_set_width(s_player_dd_alarm, LV_PCT(100));
    lv_dropdown_set_options(s_player_dd_alarm, "Home group\nKitchen\nBedroom");
    lv_obj_set_style_bg_color(s_player_dd_alarm, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(s_player_dd_alarm, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_player_dd_alarm, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(s_player_dd_alarm, 1, 0);
    lv_obj_set_style_text_font(s_player_dd_alarm, UI_FONT_UI_12, 0);
    lv_obj_set_style_text_color(s_player_dd_alarm, UI_CLR_TEXT2, 0);
    /* TODO: on LV_EVENT_VALUE_CHANGED, update player_id used in _confirm */

    /* Confirm button */
    lv_obj_t *confirm = lv_obj_create(card);
    lv_obj_set_size(confirm, LV_PCT(100), 48);
    lv_obj_set_style_bg_color(confirm, UI_CLR_ACCENT, 0);
    lv_obj_set_style_bg_opa(confirm, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(confirm, 0, 0);
    lv_obj_set_style_radius(confirm, 12, 0);
    lv_obj_set_layout(confirm, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(confirm, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(confirm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(confirm, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(confirm, _confirm, LV_EVENT_CLICKED, NULL);

    s_confirm_lbl = lv_label_create(confirm);
    lv_obj_set_style_text_font(s_confirm_lbl, UI_FONT_UI_14, 0);
    lv_obj_set_style_text_color(s_confirm_lbl, UI_CLR_BG, 0);
    lv_label_set_text_fmt(s_confirm_lbl, "Set for %02d:%02d", s_hh, s_mm);

    /* Upcoming alarms section (only if any exist) */
    if (s_alarm_n > 0) {
        lv_obj_t *div = lv_obj_create(card);
        lv_obj_set_size(div, LV_PCT(100), 1);
        lv_obj_set_style_bg_color(div, UI_CLR_BORDER, 0);
        lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(div, 0, 0);
        lv_obj_set_style_pad_all(div, 0, 0);

        lv_obj_t *sec_hdr = lv_label_create(card);
        lv_obj_set_style_text_font(sec_hdr, UI_FONT_MONO_10, 0);
        lv_obj_set_style_text_color(sec_hdr, UI_CLR_TEXT3, 0);
        lv_obj_set_style_text_letter_space(sec_hdr, 2, 0);
        lv_label_set_text(sec_hdr, "UPCOMING");

        s_alarm_list = lv_obj_create(card);
        lv_obj_set_size(s_alarm_list, LV_PCT(100), LV_SIZE_CONTENT);
        ui_style_ghost(s_alarm_list);
        lv_obj_set_layout(s_alarm_list, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(s_alarm_list, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(s_alarm_list, 8, 0);

        _rebuild_alarm_list();
    } else {
        s_alarm_list = NULL;
    }
}

/* ════════════════════════════════════════════════════════════════════════
 * Ringing popup
 * ════════════════════════════════════════════════════════════════════════ */
static lv_obj_t *s_ring_overlay = NULL;

#define MAX_RINGING 4
static ui_ringing_entry_t s_ringing[MAX_RINGING];
static int                 s_ring_n = 0;

static void _stop_ringing(lv_event_t *e) {
    (void)e;
    for (int i = 0; i < s_ring_n; i++) {
        /* TODO: mqtt publish dashboard/cmd/stop_alarm (or stop_timer) with s_ringing[i].id */
    }
    if (s_ring_overlay) {
        lv_obj_del(s_ring_overlay);
        s_ring_overlay = NULL;
    }
    s_ring_n = 0;
}

void ui_update_ringing(const ui_ringing_entry_t *entries, int count) {
    int n = count < MAX_RINGING ? count : MAX_RINGING;
    if (n > 0) memcpy(s_ringing, entries, n * sizeof(ui_ringing_entry_t));
    s_ring_n = n;

    if (n == 0) {
        if (s_ring_overlay) { lv_obj_del(s_ring_overlay); s_ring_overlay = NULL; }
        return;
    }

    if (s_ring_overlay) return;   /* already shown */

    bool is_alarm = (strcmp(s_ringing[0].kind, "alarm") == 0);
    lv_color_t accent = is_alarm ? UI_CLR_WARM   : UI_CLR_ACCENT;
    lv_color_t abg    = is_alarm ? UI_CLR_WARM_BG : UI_CLR_ACCENT_BG;

    /* Full-screen overlay — highest layer */
    s_ring_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_ring_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_ring_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_ring_overlay, UI_OPA_RINGING, 0);
    lv_obj_set_style_border_width(s_ring_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_ring_overlay, 0, 0);
    lv_obj_set_layout(s_ring_overlay, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(s_ring_overlay, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_ring_overlay, LV_OBJ_FLAG_SCROLLABLE);

    /* Card */
    lv_obj_t *card = lv_obj_create(s_ring_overlay);
    lv_obj_set_size(card, 380, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, abg, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, accent, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 26, 0);
    lv_obj_set_style_pad_all(card, 48, 0);
    lv_obj_set_style_pad_hor(card, 64, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 18, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Kind label */
    lv_obj_t *kind_lbl = lv_label_create(card);
    lv_obj_set_style_text_font(kind_lbl, UI_FONT_MONO_18, 0);
    lv_obj_set_style_text_color(kind_lbl, accent, 0);
    lv_obj_set_style_text_letter_space(kind_lbl, 5, 0);
    lv_label_set_text(kind_lbl, is_alarm ? "ALARM" : "TIMER DONE");

    /* Player label */
    lv_obj_t *player_lbl = lv_label_create(card);
    lv_obj_set_style_text_font(player_lbl, UI_FONT_MONO_12, 0);
    lv_obj_set_style_text_color(player_lbl, UI_CLR_TEXT2, 0);
    lv_label_set_text(player_lbl, s_ringing[0].player_id);

    /* STOP button */
    lv_obj_t *stop = lv_obj_create(card);
    lv_obj_set_size(stop, LV_PCT(100), 62);
    lv_obj_set_style_margin_top(stop, 10, 0);
    lv_obj_set_style_bg_color(stop, accent, 0);
    lv_obj_set_style_bg_opa(stop, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(stop, 0, 0);
    lv_obj_set_style_radius(stop, 16, 0);
    lv_obj_set_layout(stop, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(stop, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(stop, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(stop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(stop, _stop_ringing, LV_EVENT_CLICKED, NULL);

    lv_obj_t *stop_lbl = lv_label_create(stop);
    lv_obj_set_style_text_font(stop_lbl, UI_FONT_MONO_18, 0);
    lv_obj_set_style_text_color(stop_lbl, UI_CLR_BG, 0);
    lv_obj_set_style_text_letter_space(stop_lbl, 5, 0);
    lv_label_set_text(stop_lbl, "STOP");

    /* "+N more" */
    if (s_ring_n > 1) {
        lv_obj_t *more = lv_label_create(card);
        lv_obj_set_style_text_font(more, UI_FONT_MONO_11, 0);
        lv_obj_set_style_text_color(more, UI_CLR_TEXT3, 0);
        lv_label_set_text_fmt(more, "+%d more ringing", s_ring_n - 1);
    }
}
