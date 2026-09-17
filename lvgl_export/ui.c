/* ui.c — Top-level init: screen, custom tab bar, style helpers
 * LVGL v8.3  |  1280×800 landscape  |  ESP32-P4
 */
#include "ui.h"
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════
 * Tab state
 * ═══════════════════════════════════════════════════════════════════════ */
#define TAB_COUNT 3
static lv_obj_t *s_tab_btn[TAB_COUNT];
static lv_obj_t *s_tab_lbl[TAB_COUNT];
static lv_obj_t *s_tab_panel[TAB_COUNT];
static int        s_active = 0;

static const char *TAB_NAMES[TAB_COUNT] = {
    "Clock & Weather",
    "Home",
    "Music",
};

/* ═══════════════════════════════════════════════════════════════════════
 * Shared style helpers
 * ═══════════════════════════════════════════════════════════════════════ */
void ui_style_card(lv_obj_t *obj) {
    lv_obj_set_style_bg_color(obj, UI_CLR_SURF2, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, 16, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void ui_style_ghost(lv_obj_t *obj) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void ui_style_scroll_col(lv_obj_t *obj) {
    lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(obj, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_scrollbar_width(obj, 3, 0);
    lv_obj_set_style_scrollbar_color(obj, UI_CLR_BORDER_BR, 0);
    lv_obj_set_style_scrollbar_opa(obj, LV_OPA_COVER, 0);
}

/* ═══════════════════════════════════════════════════════════════════════
 * Tab button event
 * ═══════════════════════════════════════════════════════════════════════ */
typedef struct { int idx; } _tab_ud_t;
static _tab_ud_t _tab_ud[TAB_COUNT] = {{0},{1},{2}};

static void _tab_btn_cb(lv_event_t *e) {
    _tab_ud_t *d = (_tab_ud_t *)lv_event_get_user_data(e);
    int idx = d->idx;
    if (idx == s_active) return;

    /* Deactivate old */
    lv_obj_set_style_bg_opa(s_tab_btn[s_active], LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(s_tab_lbl[s_active], UI_CLR_TEXT2, 0);
    lv_obj_add_flag(s_tab_panel[s_active], LV_OBJ_FLAG_HIDDEN);

    /* Activate new */
    lv_obj_set_style_bg_color(s_tab_btn[idx], UI_CLR_ACCENT, 0);
    lv_obj_set_style_bg_opa(s_tab_btn[idx], LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_tab_lbl[idx], UI_CLR_BG, 0);
    lv_obj_clear_flag(s_tab_panel[idx], LV_OBJ_FLAG_HIDDEN);

    s_active = idx;
}

/* ═══════════════════════════════════════════════════════════════════════
 * Tab bar
 * ═══════════════════════════════════════════════════════════════════════ */
static void _create_tab_bar(lv_obj_t *root) {
    /* Full-width bar row — 68 px tall */
    lv_obj_t *bar = lv_obj_create(root);
    lv_obj_set_size(bar, LV_PCT(100), 68);
    lv_obj_set_style_bg_color(bar, UI_CLR_SURF, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bar, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_layout(bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Pill container */
    lv_obj_t *pill = lv_obj_create(bar);
    lv_obj_set_size(pill, LV_SIZE_CONTENT, 50);
    lv_obj_set_style_bg_color(pill, UI_CLR_SURF2, 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(pill, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(pill, 1, 0);
    lv_obj_set_style_radius(pill, 14, 0);
    lv_obj_set_style_pad_all(pill, 4, 0);
    lv_obj_set_style_pad_column(pill, 4, 0);
    lv_obj_set_layout(pill, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pill, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < TAB_COUNT; i++) {
        lv_obj_t *btn = lv_obj_create(pill);
        lv_obj_set_size(btn, LV_SIZE_CONTENT, 42);
        lv_obj_set_style_radius(btn, 10, 0);
        lv_obj_set_style_pad_hor(btn, 22, 0);
        lv_obj_set_style_pad_ver(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, _tab_btn_cb, LV_EVENT_CLICKED, &_tab_ud[i]);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        if (i == 0) {
            lv_obj_set_style_bg_color(btn, UI_CLR_ACCENT, 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        }

        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_style_text_font(lbl, UI_FONT_UI_14, 0);
        lv_obj_set_style_text_color(lbl, i == 0 ? UI_CLR_BG : UI_CLR_TEXT2, 0);
        lv_label_set_text(lbl, TAB_NAMES[i]);

        s_tab_btn[i] = btn;
        s_tab_lbl[i] = lbl;
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 * ui_init
 * ═══════════════════════════════════════════════════════════════════════ */
void ui_init(void) {
    /* Screen background */
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, UI_CLR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* Root: full-screen flex column */
    lv_obj_t *root = lv_obj_create(scr);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    ui_style_ghost(root);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 0, 0);

    _create_tab_bar(root);

    /* Content area: grows to fill remaining height below the tab bar */
    lv_obj_t *content = lv_obj_create(root);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);
    ui_style_ghost(content);

    /* Three stacked panels, only panel[0] visible at start */
    for (int i = 0; i < TAB_COUNT; i++) {
        lv_obj_t *panel = lv_obj_create(content);
        lv_obj_set_size(panel, LV_PCT(100), LV_PCT(100));
        lv_obj_set_pos(panel, 0, 0);
        lv_obj_set_style_bg_color(panel, UI_CLR_BG, 0);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(panel, 0, 0);
        lv_obj_set_style_pad_all(panel, 0, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        if (i != 0) lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
        s_tab_panel[i] = panel;
    }

    ui_clock_build(s_tab_panel[0]);
    ui_home_build(s_tab_panel[1]);
    ui_music_build(s_tab_panel[2]);
}
