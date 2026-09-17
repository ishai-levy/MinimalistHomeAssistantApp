/* ui_home.c — Home tab
 * 3×2 grid of cards:
 *   [0,0] Roborock vacuum (Start / Stop / Dock buttons)
 *   [1,0] The Dude toggle
 *   [2,0] Find Phone tap
 *   [0,1] Mirror / Camera tap
 *   [1,1] Alarm card  →  opens AlarmModal overlay
 *   [2,1] Timer card  (arc ring + presets + player dropdown)
 */
#include "ui.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

/* ── Timer state ────────────────────────────────────────────────────── */
static int          s_timer_dur  = 300;   /* seconds */
static int          s_timer_rem  = 300;
static bool         s_timer_run  = false;
static lv_timer_t  *s_timer_lv   = NULL;
static lv_obj_t    *s_arc;
static lv_obj_t    *s_timer_lbl;
static lv_obj_t    *s_timer_start_btn;
static lv_obj_t    *s_timer_start_lbl;
static lv_obj_t    *s_timer_status_lbl;
static lv_obj_t    *s_timer_player_dd;

/* ── Other widget handles ───────────────────────────────────────────── */
static lv_obj_t *s_robo_status;
static lv_obj_t *s_dude_card;
static lv_obj_t *s_dude_status;
static bool      s_dude_on = false;
static lv_obj_t *s_alarm_next;

/* ══════════════════════════════════════════════════════════════════════
 * Timer helpers
 * ══════════════════════════════════════════════════════════════════════ */
static void _timer_refresh_display(void) {
    lv_label_set_text_fmt(s_timer_lbl, "%02d:%02d",
        s_timer_rem / 60, s_timer_rem % 60);
    int deg = (s_timer_dur > 0)
        ? (int)((float)s_timer_rem / (float)s_timer_dur * 360.0f)
        : 0;
    /* LVGL arc: value maps linearly over [bg_start_angle, bg_end_angle].
     * We set range 0-360 and fill clockwise from top. */
    lv_arc_set_value(s_arc, deg);
}

static void _set_start_btn_style(bool running, bool done) {
    lv_color_t bg, border, text;
    const char *label;
    if (done) {
        bg = UI_CLR_WARM_BG; border = UI_CLR_WARM; text = UI_CLR_WARM;
        label = "Restart";
    } else if (running) {
        bg = UI_CLR_WARM_BG; border = UI_CLR_WARM; text = UI_CLR_WARM;
        label = "Pause";
    } else {
        bg = UI_CLR_ACCENT_BG; border = UI_CLR_ACCENT; text = UI_CLR_ACCENT;
        label = "Start";
    }
    lv_obj_set_style_bg_color(s_timer_start_btn, bg, 0);
    lv_obj_set_style_border_color(s_timer_start_btn, border, 0);
    lv_obj_set_style_text_color(s_timer_start_lbl, text, 0);
    lv_label_set_text(s_timer_start_lbl, label);
}

static void _timer_tick(lv_timer_t *t) {
    (void)t;
    if (s_timer_rem > 0) s_timer_rem--;

    if (s_timer_rem == 0) {
        s_timer_run = false;
        lv_timer_del(s_timer_lv);
        s_timer_lv = NULL;
        lv_label_set_text(s_timer_status_lbl, "DONE");
        lv_obj_set_style_text_color(s_timer_status_lbl, UI_CLR_WARM, 0);
        _set_start_btn_style(false, true);

        /* Simulate ringing (real app: wait for dashboard/state/ringing MQTT push) */
        ui_ringing_entry_t ring;
        memset(&ring, 0, sizeof(ring));
        strncpy(ring.id,   "local_timer_0", sizeof(ring.id) - 1);
        strncpy(ring.kind, "timer",         sizeof(ring.kind) - 1);
        /* Read selected player from dropdown */
        uint16_t sel = lv_dropdown_get_selected(s_timer_player_dd);
        char player_name[64];
        lv_dropdown_get_selected_str(s_timer_player_dd, player_name, sizeof(player_name));
        strncpy(ring.player_id, player_name, sizeof(ring.player_id) - 1);
        ui_update_ringing(&ring, 1);
    }
    _timer_refresh_display();
}

/* ── Timer button events ────────────────────────────────────────────── */
static void _start_btn_cb(lv_event_t *e) {
    (void)e;
    if (s_timer_rem == 0) {
        /* Restart */
        s_timer_rem = s_timer_dur;
        s_timer_run = true;
    } else {
        s_timer_run = !s_timer_run;
    }

    if (s_timer_run) {
        if (!s_timer_lv)
            s_timer_lv = lv_timer_create(_timer_tick, 1000, NULL);
        else
            lv_timer_resume(s_timer_lv);
        lv_label_set_text(s_timer_status_lbl, "RUNNING");
        lv_obj_set_style_text_color(s_timer_status_lbl, UI_CLR_ACCENT, 0);
        _set_start_btn_style(true, false);
        /* TODO: mqtt publish {cmd:"set_timer", duration_seconds:s_timer_dur, player_id:...} */
    } else {
        if (s_timer_lv) lv_timer_pause(s_timer_lv);
        lv_label_set_text(s_timer_status_lbl, "PAUSED");
        lv_obj_set_style_text_color(s_timer_status_lbl, UI_CLR_TEXT3, 0);
        _set_start_btn_style(false, false);
    }
}

static void _reset_btn_cb(lv_event_t *e) {
    (void)e;
    s_timer_run = false;
    s_timer_rem = s_timer_dur;
    if (s_timer_lv) { lv_timer_del(s_timer_lv); s_timer_lv = NULL; }
    lv_label_set_text(s_timer_status_lbl, "READY");
    lv_obj_set_style_text_color(s_timer_status_lbl, UI_CLR_TEXT3, 0);
    _set_start_btn_style(false, false);
    _timer_refresh_display();
}

typedef struct { int secs; } _preset_t;
static _preset_t _presets[4] = {{60},{300},{600},{900}};

static void _preset_cb(lv_event_t *e) {
    _preset_t *p = (_preset_t *)lv_event_get_user_data(e);
    s_timer_dur = p->secs;
    s_timer_rem = p->secs;
    s_timer_run = false;
    if (s_timer_lv) { lv_timer_del(s_timer_lv); s_timer_lv = NULL; }
    lv_label_set_text(s_timer_status_lbl, "READY");
    lv_obj_set_style_text_color(s_timer_status_lbl, UI_CLR_TEXT3, 0);
    _set_start_btn_style(false, false);
    _timer_refresh_display();
}

/* ── Vacuum button events ───────────────────────────────────────────── */
typedef struct { const char *cmd; } _vcmd_t;
static _vcmd_t _vcmds[3] = {{"vacuum_start"},{"vacuum_stop"},{"vacuum_home"}};

static void _vac_btn_cb(lv_event_t *e) {
    _vcmd_t *d = (_vcmd_t *)lv_event_get_user_data(e);
    /* TODO: mqtt publish dashboard/cmd/{d->cmd} */
    (void)d;
}

/* ── Dude toggle ────────────────────────────────────────────────────── */
static void _dude_cb(lv_event_t *e) {
    (void)e;
    s_dude_on = !s_dude_on;
    lv_label_set_text(s_dude_status, s_dude_on ? "ON" : "OFF");
    lv_obj_set_style_text_color(s_dude_status,
        s_dude_on ? UI_CLR_ACCENT : UI_CLR_TEXT3, 0);
    lv_obj_set_style_bg_color(s_dude_card,
        s_dude_on ? UI_CLR_ACCENT_BG : UI_CLR_SURF2, 0);
    lv_obj_set_style_border_color(s_dude_card,
        s_dude_on ? UI_CLR_ACCENT : UI_CLR_BORDER, 0);
    /* TODO: mqtt publish dashboard/cmd/toggle_dude */
}

/* ── Find phone ─────────────────────────────────────────────────────── */
static void _phone_cb(lv_event_t *e) {
    (void)e;
    /* TODO: mqtt publish dashboard/cmd/find_phone */
}

/* ── Alarm card tap ─────────────────────────────────────────────────── */
static void _alarm_card_cb(lv_event_t *e) {
    (void)e;
    ui_overlay_show_alarm_modal();
}

/* ══════════════════════════════════════════════════════════════════════
 * Card factory helpers
 * ══════════════════════════════════════════════════════════════════════ */
static lv_obj_t *_make_card(lv_obj_t *grid, int col, int row) {
    lv_obj_t *c = lv_obj_create(grid);
    ui_style_card(c);
    lv_obj_set_style_pad_all(c, 22, 0);
    lv_obj_set_style_pad_bottom(c, 18, 0);
    lv_obj_set_grid_cell(c, LV_GRID_ALIGN_STRETCH, col, 1,
                              LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_layout(c, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_SPACE_BETWEEN,
                            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
}

static lv_obj_t *_chip_btn(lv_obj_t *parent, const char *text) {
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, LV_SIZE_CONTENT, 26);
    lv_obj_set_style_bg_color(c, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(c, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_radius(c, 6, 0);
    lv_obj_set_style_pad_hor(c, 8, 0);
    lv_obj_set_style_pad_ver(c, 0, 0);
    lv_obj_set_layout(c, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *l = lv_label_create(c);
    lv_obj_set_style_text_font(l, UI_FONT_MONO_10, 0);
    lv_obj_set_style_text_color(l, UI_CLR_TEXT3, 0);
    lv_label_set_text(l, text);
    return c;
}

static lv_obj_t *_action_btn(lv_obj_t *parent,
                               lv_color_t bg, lv_color_t border, lv_color_t text,
                               const char *label) {
    lv_obj_t *b = lv_obj_create(parent);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_height(b, 40);
    lv_obj_set_style_bg_color(b, bg, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b, border, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_radius(b, 9, 0);
    lv_obj_set_style_pad_all(b, 0, 0);
    lv_obj_set_layout(b, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_style_text_font(l, UI_FONT_UI_12, 0);
    lv_obj_set_style_text_color(l, text, 0);
    lv_label_set_text(l, label);
    return b;
}

/* ══════════════════════════════════════════════════════════════════════
 * ui_home_build
 * ══════════════════════════════════════════════════════════════════════ */
void ui_home_build(lv_obj_t *parent) {
    /* Padding container */
    lv_obj_t *wrap = lv_obj_create(parent);
    lv_obj_set_size(wrap, LV_PCT(100), LV_PCT(100));
    ui_style_ghost(wrap);
    lv_obj_set_style_pad_all(wrap, 20, 0);
    lv_obj_set_style_pad_hor(wrap, 36, 0);

    /* 3×2 grid */
    static lv_coord_t cols[] = {LV_GRID_FR(1),LV_GRID_FR(1),LV_GRID_FR(1),LV_GRID_SENTINEL};
    static lv_coord_t rows[] = {LV_GRID_FR(1),LV_GRID_FR(1),LV_GRID_SENTINEL};

    lv_obj_t *grid = lv_obj_create(wrap);
    lv_obj_set_size(grid, LV_PCT(100), LV_PCT(100));
    ui_style_ghost(grid);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, cols, rows);
    lv_obj_set_style_pad_gap(grid, 16, 0);

    /* ──────────────────────────────────────────────────────────────────
     * [0,0] Roborock
     * ────────────────────────────────────────────────────────────────── */
    lv_obj_t *robo = _make_card(grid, 0, 0);

    lv_obj_t *robo_top = lv_obj_create(robo);
    lv_obj_set_size(robo_top, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(robo_top);
    lv_obj_set_layout(robo_top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(robo_top, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(robo_top, 4, 0);

    lv_obj_t *robo_name = lv_label_create(robo_top);
    lv_obj_set_style_text_font(robo_name, UI_FONT_UI_16, 0);
    lv_obj_set_style_text_color(robo_name, UI_CLR_TEXT, 0);
    lv_label_set_text(robo_name, "Roborock");

    s_robo_status = lv_label_create(robo_top);
    lv_obj_set_style_text_font(s_robo_status, UI_FONT_MONO_11, 0);
    lv_obj_set_style_text_color(s_robo_status, UI_CLR_TEXT3, 0);
    lv_label_set_text(s_robo_status, "docked");

    /* Sub-buttons row */
    lv_obj_t *vac_row = lv_obj_create(robo);
    lv_obj_set_size(vac_row, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(vac_row);
    lv_obj_set_layout(vac_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(vac_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vac_row, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(vac_row, 8, 0);

    const char *vac_labels[] = {"Start","Stop","Dock"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *b = _action_btn(vac_row,
            UI_CLR_SURF3, UI_CLR_BORDER, UI_CLR_TEXT2, vac_labels[i]);
        lv_obj_add_event_cb(b, _vac_btn_cb, LV_EVENT_CLICKED, &_vcmds[i]);
    }

    /* ──────────────────────────────────────────────────────────────────
     * [1,0] The Dude toggle
     * ────────────────────────────────────────────────────────────────── */
    s_dude_card = _make_card(grid, 1, 0);
    lv_obj_add_flag(s_dude_card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_dude_card, _dude_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *dude_spacer = lv_obj_create(s_dude_card);
    lv_obj_set_flex_grow(dude_spacer, 1);
    ui_style_ghost(dude_spacer);

    lv_obj_t *dude_bot = lv_obj_create(s_dude_card);
    lv_obj_set_size(dude_bot, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(dude_bot);
    lv_obj_set_layout(dude_bot, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dude_bot, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(dude_bot, 4, 0);

    lv_obj_t *dude_name = lv_label_create(dude_bot);
    lv_obj_set_style_text_font(dude_name, UI_FONT_UI_16, 0);
    lv_obj_set_style_text_color(dude_name, UI_CLR_TEXT, 0);
    lv_label_set_text(dude_name, "The Dude");

    s_dude_status = lv_label_create(dude_bot);
    lv_obj_set_style_text_font(s_dude_status, UI_FONT_MONO_11, 0);
    lv_obj_set_style_text_color(s_dude_status, UI_CLR_TEXT3, 0);
    lv_label_set_text(s_dude_status, "OFF");

    /* ──────────────────────────────────────────────────────────────────
     * [2,0] Find Phone
     * ────────────────────────────────────────────────────────────────── */
    lv_obj_t *phone = _make_card(grid, 2, 0);
    lv_obj_add_flag(phone, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(phone, _phone_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *phone_sp = lv_obj_create(phone);
    lv_obj_set_flex_grow(phone_sp, 1);
    ui_style_ghost(phone_sp);

    lv_obj_t *phone_name = lv_label_create(phone);
    lv_obj_set_style_text_font(phone_name, UI_FONT_UI_16, 0);
    lv_obj_set_style_text_color(phone_name, UI_CLR_TEXT, 0);
    lv_label_set_text(phone_name, "Find Phone");

    /* ──────────────────────────────────────────────────────────────────
     * [0,1] Mirror / Camera  (placeholder)
     * ────────────────────────────────────────────────────────────────── */
    lv_obj_t *mirror = _make_card(grid, 0, 1);
    lv_obj_add_flag(mirror, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *mirror_sp = lv_obj_create(mirror);
    lv_obj_set_flex_grow(mirror_sp, 1);
    ui_style_ghost(mirror_sp);

    lv_obj_t *mirror_name = lv_label_create(mirror);
    lv_obj_set_style_text_font(mirror_name, UI_FONT_UI_16, 0);
    lv_obj_set_style_text_color(mirror_name, UI_CLR_TEXT, 0);
    lv_label_set_text(mirror_name, "Mirror");

    /* ──────────────────────────────────────────────────────────────────
     * [1,1] Alarm card
     * ────────────────────────────────────────────────────────────────── */
    lv_obj_t *alarm_card = _make_card(grid, 1, 1);
    lv_obj_add_flag(alarm_card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(alarm_card, _alarm_card_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *alarm_sp = lv_obj_create(alarm_card);
    lv_obj_set_flex_grow(alarm_sp, 1);
    ui_style_ghost(alarm_sp);

    lv_obj_t *alarm_bot = lv_obj_create(alarm_card);
    lv_obj_set_size(alarm_bot, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(alarm_bot);
    lv_obj_set_layout(alarm_bot, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(alarm_bot, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(alarm_bot, 4, 0);

    lv_obj_t *alarm_name = lv_label_create(alarm_bot);
    lv_obj_set_style_text_font(alarm_name, UI_FONT_UI_16, 0);
    lv_obj_set_style_text_color(alarm_name, UI_CLR_TEXT, 0);
    lv_label_set_text(alarm_name, "Set Alarm");

    s_alarm_next = lv_label_create(alarm_bot);
    lv_obj_set_style_text_font(s_alarm_next, UI_FONT_MONO_11, 0);
    lv_obj_set_style_text_color(s_alarm_next, UI_CLR_TEXT3, 0);
    lv_label_set_text(s_alarm_next, "NONE SET");

    /* ──────────────────────────────────────────────────────────────────
     * [2,1] Timer card
     * ────────────────────────────────────────────────────────────────── */
    lv_obj_t *tc = lv_obj_create(grid);
    ui_style_card(tc);
    lv_obj_set_style_pad_all(tc, 18, 0);
    lv_obj_set_style_pad_hor(tc, 20, 0);
    lv_obj_set_style_pad_bottom(tc, 14, 0);
    lv_obj_set_grid_cell(tc, LV_GRID_ALIGN_STRETCH, 2, 1,
                              LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_layout(tc, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tc, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tc, LV_FLEX_ALIGN_SPACE_BETWEEN,
                            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(tc, 8, 0);

    /* Header row: title + preset chips */
    lv_obj_t *tc_hdr = lv_obj_create(tc);
    lv_obj_set_size(tc_hdr, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(tc_hdr);
    lv_obj_set_layout(tc_hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tc_hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tc_hdr, LV_FLEX_ALIGN_SPACE_BETWEEN,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    lv_obj_t *tc_title_grp = lv_obj_create(tc_hdr);
    ui_style_ghost(tc_title_grp);
    lv_obj_set_size(tc_title_grp, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(tc_title_grp, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tc_title_grp, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(tc_title_grp, 3, 0);

    lv_obj_t *tc_name = lv_label_create(tc_title_grp);
    lv_obj_set_style_text_font(tc_name, UI_FONT_UI_14, 0);
    lv_obj_set_style_text_color(tc_name, UI_CLR_TEXT, 0);
    lv_label_set_text(tc_name, "Timer");

    s_timer_status_lbl = lv_label_create(tc_title_grp);
    lv_obj_set_style_text_font(s_timer_status_lbl, UI_FONT_MONO_10, 0);
    lv_obj_set_style_text_color(s_timer_status_lbl, UI_CLR_TEXT3, 0);
    lv_obj_set_style_text_letter_space(s_timer_status_lbl, 1, 0);
    lv_label_set_text(s_timer_status_lbl, "READY");

    lv_obj_t *chips_row = lv_obj_create(tc_hdr);
    ui_style_ghost(chips_row);
    lv_obj_set_size(chips_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(chips_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(chips_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(chips_row, 4, 0);

    const char *preset_labels[] = {"1m","5m","10m","15m"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *chip = _chip_btn(chips_row, preset_labels[i]);
        lv_obj_add_event_cb(chip, _preset_cb, LV_EVENT_CLICKED, &_presets[i]);
    }

    /* Arc ring + readout */
    lv_obj_t *arc_wrap = lv_obj_create(tc);
    lv_obj_set_flex_grow(arc_wrap, 1);
    lv_obj_set_width(arc_wrap, LV_PCT(100));
    ui_style_ghost(arc_wrap);
    lv_obj_set_layout(arc_wrap, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(arc_wrap, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(arc_wrap, 0, 0);

    s_arc = lv_arc_create(arc_wrap);
    lv_obj_set_size(s_arc, 106, 106);
    lv_arc_set_range(s_arc, 0, 360);
    lv_arc_set_value(s_arc, 360);
    lv_arc_set_bg_angles(s_arc, 0, 360);
    lv_arc_set_rotation(s_arc, 270);
    lv_obj_set_style_arc_color(s_arc, UI_CLR_SURF3, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_arc, UI_CLR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 4, LV_PART_INDICATOR);
    lv_obj_remove_style(s_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);

    /* Time label overlaid on arc using align */
    s_timer_lbl = lv_label_create(arc_wrap);
    lv_obj_set_style_text_font(s_timer_lbl, UI_FONT_MONO_32, 0);
    lv_obj_set_style_text_color(s_timer_lbl, UI_CLR_TEXT, 0);
    lv_obj_align_to(s_timer_lbl, s_arc, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(s_timer_lbl, "05:00");

    /* Player dropdown */
    s_timer_player_dd = lv_dropdown_create(tc);
    lv_obj_set_width(s_timer_player_dd, LV_PCT(100));
    lv_dropdown_set_options(s_timer_player_dd, "Home group\nKitchen\nBedroom");
    lv_obj_set_style_bg_color(s_timer_player_dd, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(s_timer_player_dd, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_timer_player_dd, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(s_timer_player_dd, 1, 0);
    lv_obj_set_style_text_font(s_timer_player_dd, UI_FONT_UI_12, 0);
    lv_obj_set_style_text_color(s_timer_player_dd, UI_CLR_TEXT2, 0);
    /* TODO: on LV_EVENT_VALUE_CHANGED, read selection for MQTT publish */

    /* Control buttons */
    lv_obj_t *ctrl = lv_obj_create(tc);
    lv_obj_set_size(ctrl, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(ctrl);
    lv_obj_set_layout(ctrl, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrl, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(ctrl, 6, 0);

    s_timer_start_btn = lv_obj_create(ctrl);
    lv_obj_set_flex_grow(s_timer_start_btn, 2);
    lv_obj_set_height(s_timer_start_btn, 38);
    lv_obj_set_style_bg_color(s_timer_start_btn, UI_CLR_ACCENT_BG, 0);
    lv_obj_set_style_bg_opa(s_timer_start_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_timer_start_btn, UI_CLR_ACCENT, 0);
    lv_obj_set_style_border_width(s_timer_start_btn, 1, 0);
    lv_obj_set_style_radius(s_timer_start_btn, 9, 0);
    lv_obj_set_layout(s_timer_start_btn, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(s_timer_start_btn, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(s_timer_start_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_timer_start_btn, _start_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_clear_flag(s_timer_start_btn, LV_OBJ_FLAG_SCROLLABLE);

    s_timer_start_lbl = lv_label_create(s_timer_start_btn);
    lv_obj_set_style_text_font(s_timer_start_lbl, UI_FONT_UI_12, 0);
    lv_obj_set_style_text_color(s_timer_start_lbl, UI_CLR_ACCENT, 0);
    lv_label_set_text(s_timer_start_lbl, "Start");

    lv_obj_t *reset_btn = lv_obj_create(ctrl);
    lv_obj_set_flex_grow(reset_btn, 1);
    lv_obj_set_height(reset_btn, 38);
    lv_obj_set_style_bg_color(reset_btn, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(reset_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(reset_btn, UI_CLR_BORDER_BR, 0);
    lv_obj_set_style_border_width(reset_btn, 1, 0);
    lv_obj_set_style_radius(reset_btn, 9, 0);
    lv_obj_set_layout(reset_btn, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(reset_btn, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(reset_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(reset_btn, _reset_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_clear_flag(reset_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *rl = lv_label_create(reset_btn);
    lv_obj_set_style_text_font(rl, UI_FONT_UI_12, 0);
    lv_obj_set_style_text_color(rl, UI_CLR_TEXT2, 0);
    lv_label_set_text(rl, "Reset");
}

/* ══════════════════════════════════════════════════════════════════════
 * State update functions
 * ══════════════════════════════════════════════════════════════════════ */
void ui_update_dude(bool on) {
    s_dude_on = on;
    lv_label_set_text(s_dude_status, on ? "ON" : "OFF");
    lv_obj_set_style_text_color(s_dude_status,
        on ? UI_CLR_ACCENT : UI_CLR_TEXT3, 0);
    lv_obj_set_style_bg_color(s_dude_card,
        on ? UI_CLR_ACCENT_BG : UI_CLR_SURF2, 0);
    lv_obj_set_style_border_color(s_dude_card,
        on ? UI_CLR_ACCENT : UI_CLR_BORDER, 0);
}

void ui_update_vacuum(const char *state) {
    lv_label_set_text(s_robo_status, state);
    bool active = (strcmp(state,"cleaning")==0 || strcmp(state,"returning")==0);
    lv_obj_set_style_text_color(s_robo_status,
        active ? UI_CLR_SUCCESS : UI_CLR_TEXT3, 0);
}

void ui_update_alarms(const ui_schedule_entry_t *entries, int count) {
    if (count == 0) {
        lv_label_set_text(s_alarm_next, "NONE SET");
        lv_obj_set_style_text_color(s_alarm_next, UI_CLR_TEXT3, 0);
    } else {
        time_t t = (time_t)entries[0].trigger_at;
        struct tm *tm = localtime(&t);
        int h12 = tm->tm_hour % 12;
        if (!h12) h12 = 12;
        lv_label_set_text_fmt(s_alarm_next, "-> %02d:%02d %s",
            h12, tm->tm_min,
            tm->tm_hour >= 12 ? "PM" : "AM");
        lv_obj_set_style_text_color(s_alarm_next, UI_CLR_ACCENT, 0);
    }
}

void ui_update_timers(const ui_schedule_entry_t *entries, int count) {
    /* Backend-managed timers are shown here when they come in via MQTT.
     * The local arc handles the countdown independently. */
    (void)entries; (void)count;
}
