/* ui_clock.c — Clock & Weather tab
 * Layout: left half = clock + current weather detail grid
 *         right half = 3-day forecast + 3-hour forecast
 */
#include "ui.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ── Widget handles ─────────────────────────────────────────────────── */
static lv_obj_t *s_lbl_time;
static lv_obj_t *s_lbl_ampm;
static lv_obj_t *s_lbl_day;
static lv_obj_t *s_lbl_date;
static lv_obj_t *s_lbl_temp;
static lv_obj_t *s_lbl_cond;
static lv_obj_t *s_lbl_humidity;
static lv_obj_t *s_lbl_wind;
static lv_obj_t *s_lbl_pressure;
static lv_obj_t *s_lbl_feels;

#define DAILY_COUNT 3
#define HOURLY_COUNT 3
static lv_obj_t *s_daily_day[DAILY_COUNT];
static lv_obj_t *s_daily_icon[DAILY_COUNT];
static lv_obj_t *s_daily_high[DAILY_COUNT];
static lv_obj_t *s_daily_low[DAILY_COUNT];
static lv_obj_t *s_hourly_hour[HOURLY_COUNT];
static lv_obj_t *s_hourly_icon[HOURLY_COUNT];
static lv_obj_t *s_hourly_temp[HOURLY_COUNT];

/* ── Simple weather icon mapping (ASCII, replace with icon font later) ─ */
static const char *_cond_icon(const char *c) {
    if (!c || !*c) return "-";
    char lc[64]; int i;
    for (i = 0; c[i] && i < 63; i++) lc[i] = (char)tolower((unsigned char)c[i]);
    lc[i] = 0;
    if (strstr(lc, "sunny")   || strstr(lc, "clear"))   return "Sunny";
    if (strstr(lc, "thunder") || strstr(lc, "storm"))   return "Storm";
    if (strstr(lc, "snow")    || strstr(lc, "sleet"))   return "Snow";
    if (strstr(lc, "rain")    || strstr(lc, "drizzle")) return "Rain";
    if (strstr(lc, "fog")     || strstr(lc, "haze"))    return "Fog";
    if (strstr(lc, "wind"))                              return "Wind";
    if (strstr(lc, "cloud")   || strstr(lc, "overcast")) return "Cloud";
    return "-";
}

static const char *_day_abbr[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char *_mon_abbr[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};

/* ── Clock timer callback ───────────────────────────────────────────── */
static void _clock_tick(lv_timer_t *t) {
    (void)t;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);

    int h12 = tm->tm_hour % 12;
    if (h12 == 0) h12 = 12;
    const char *ampm = (tm->tm_hour >= 12) ? "PM" : "AM";

    lv_label_set_text_fmt(s_lbl_time, "%02d:%02d", h12, tm->tm_min);
    lv_label_set_text(s_lbl_ampm, ampm);

    /* Day: uppercase */
    char day_buf[16];
    const char *d = _day_abbr[tm->tm_wday];
    snprintf(day_buf, sizeof(day_buf), "%s", d);
    for (char *p = day_buf; *p; p++) *p = (char)toupper((unsigned char)*p);
    lv_label_set_text(s_lbl_day, day_buf);

    lv_label_set_text_fmt(s_lbl_date, "%s %d, %d",
        _mon_abbr[tm->tm_mon], tm->tm_mday, 1900 + tm->tm_year);
}

/* ── Forecast card factory ──────────────────────────────────────────── */
static lv_obj_t *_make_fcst_card(lv_obj_t *grid, int col,
                                  lv_obj_t **out_top,
                                  lv_obj_t **out_icon,
                                  lv_obj_t **out_main,
                                  lv_obj_t **out_sub) {
    static lv_coord_t _row[] = {LV_SIZE_CONTENT, LV_GRID_SENTINEL};
    /* col_dsc is owned by the parent grid — we just place into it */

    lv_obj_t *card = lv_obj_create(grid);
    lv_obj_set_style_bg_color(card, UI_CLR_SURF2, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    *out_top = lv_label_create(card);
    lv_obj_set_style_text_font(*out_top, UI_FONT_MONO_11, 0);
    lv_obj_set_style_text_color(*out_top, UI_CLR_TEXT2, 0);
    lv_label_set_text(*out_top, "--");

    *out_icon = lv_label_create(card);
    lv_obj_set_style_text_font(*out_icon, UI_FONT_UI_13, 0);
    lv_obj_set_style_text_color(*out_icon, UI_CLR_TEXT2, 0);
    lv_label_set_text(*out_icon, "-");

    *out_main = lv_label_create(card);
    lv_obj_set_style_text_font(*out_main, UI_FONT_MONO_14, 0);
    lv_obj_set_style_text_color(*out_main, UI_CLR_TEXT, 0);
    lv_label_set_text(*out_main, "--");

    *out_sub = lv_label_create(card);
    lv_obj_set_style_text_font(*out_sub, UI_FONT_MONO_11, 0);
    lv_obj_set_style_text_color(*out_sub, UI_CLR_TEXT3, 0);
    lv_label_set_text(*out_sub, "--");

    return card;
}

/* ── Detail cell factory ────────────────────────────────────────────── */
static lv_obj_t **_det_val_lbls[4];
static lv_obj_t *_det_vals[4];

static void _make_detail_cell(lv_obj_t *grid, int col, int row,
                               const char *key, lv_obj_t **out_val) {
    lv_obj_t *cell = lv_obj_create(grid);
    lv_obj_set_style_bg_color(cell, UI_CLR_SURF3, 0);
    lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cell, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(cell, 1, 0);
    lv_obj_set_style_radius(cell, 10, 0);
    lv_obj_set_style_pad_all(cell, 12, 0);
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, col, 1,
                                LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_layout(cell, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cell, 4, 0);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *k = lv_label_create(cell);
    lv_obj_set_style_text_font(k, UI_FONT_MONO_10, 0);
    lv_obj_set_style_text_color(k, UI_CLR_TEXT3, 0);
    lv_label_set_text(k, key);

    *out_val = lv_label_create(cell);
    lv_obj_set_style_text_font(*out_val, UI_FONT_MONO_14, 0);
    lv_obj_set_style_text_color(*out_val, UI_CLR_TEXT, 0);
    lv_label_set_text(*out_val, "--");
}

/* ── Section heading factory ────────────────────────────────────────── */
static lv_obj_t *_section_heading(lv_obj_t *parent, const char *text) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, UI_FONT_MONO_10, 0);
    lv_obj_set_style_text_color(lbl, UI_CLR_TEXT3, 0);
    lv_obj_set_style_text_letter_space(lbl, 2, 0);
    lv_label_set_text(lbl, text);
    return lbl;
}

/* ══════════════════════════════════════════════════════════════════════
 * ui_clock_build
 * ══════════════════════════════════════════════════════════════════════ */
void ui_clock_build(lv_obj_t *parent) {
    /* ── Left panel ──────────────────────────────────────────────────── */
    lv_obj_t *left = lv_obj_create(parent);
    lv_obj_set_size(left, LV_PCT(50), LV_PCT(100));
    lv_obj_set_pos(left, 0, 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(left, UI_CLR_BORDER, 0);
    lv_obj_set_style_border_width(left, 1, 0);
    lv_obj_set_style_border_side(left, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_pad_hor(left, 52, 0);
    lv_obj_set_style_pad_ver(left, 0, 0);
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(left, 0, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    /* Time row: HH:MM + AM/PM superscript */
    lv_obj_t *time_row = lv_obj_create(left);
    lv_obj_set_size(time_row, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(time_row);
    lv_obj_set_layout(time_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(time_row, 12, 0);

    s_lbl_time = lv_label_create(time_row);
    lv_obj_set_style_text_font(s_lbl_time, UI_FONT_MONO_140, 0);
    lv_obj_set_style_text_color(s_lbl_time, UI_CLR_TEXT, 0);
    lv_label_set_text(s_lbl_time, "12:00");

    s_lbl_ampm = lv_label_create(time_row);
    lv_obj_set_style_text_font(s_lbl_ampm, UI_FONT_MONO_32, 0);
    lv_obj_set_style_text_color(s_lbl_ampm, UI_CLR_ACCENT, 0);
    lv_obj_set_style_pad_bottom(s_lbl_ampm, 16, 0);
    lv_label_set_text(s_lbl_ampm, "AM");

    /* Day (uppercase) */
    s_lbl_day = lv_label_create(left);
    lv_obj_set_style_text_font(s_lbl_day, UI_FONT_MONO_18, 0);
    lv_obj_set_style_text_color(s_lbl_day, UI_CLR_TEXT2, 0);
    lv_obj_set_style_text_letter_space(s_lbl_day, 4, 0);
    lv_label_set_text(s_lbl_day, "MONDAY");

    /* Date */
    s_lbl_date = lv_label_create(left);
    lv_obj_set_style_text_font(s_lbl_date, UI_FONT_MONO_14, 0);
    lv_obj_set_style_text_color(s_lbl_date, UI_CLR_TEXT3, 0);
    lv_obj_set_style_text_letter_space(s_lbl_date, 2, 0);
    lv_obj_set_style_margin_bottom(s_lbl_date, 28, 0);
    lv_label_set_text(s_lbl_date, "Jan 1, 2026");

    /* Divider */
    lv_obj_t *div = lv_obj_create(left);
    lv_obj_set_size(div, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div, UI_CLR_BORDER, 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_pad_all(div, 0, 0);
    lv_obj_set_style_margin_ver(div, 4, 0);
    lv_obj_set_style_margin_bottom(div, 28, 0);

    /* Current weather: temperature + condition */
    lv_obj_t *wx_row = lv_obj_create(left);
    lv_obj_set_size(wx_row, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(wx_row);
    lv_obj_set_layout(wx_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wx_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wx_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(wx_row, 18, 0);
    lv_obj_set_style_margin_bottom(wx_row, 24, 0);

    lv_obj_t *wx_info = lv_obj_create(wx_row);
    ui_style_ghost(wx_info);
    lv_obj_set_size(wx_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(wx_info, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wx_info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(wx_info, 4, 0);

    s_lbl_temp = lv_label_create(wx_info);
    lv_obj_set_style_text_font(s_lbl_temp, UI_FONT_MONO_48, 0);
    lv_obj_set_style_text_color(s_lbl_temp, UI_CLR_TEXT, 0);
    lv_label_set_text(s_lbl_temp, "--.-°");

    s_lbl_cond = lv_label_create(wx_info);
    lv_obj_set_style_text_font(s_lbl_cond, UI_FONT_UI_13, 0);
    lv_obj_set_style_text_color(s_lbl_cond, UI_CLR_TEXT2, 0);
    lv_label_set_text(s_lbl_cond, "Loading...");

    /* Weather detail grid: 2-col × 2-row */
    static lv_coord_t det_cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_SENTINEL};
    static lv_coord_t det_rows[] = {LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_GRID_SENTINEL};

    lv_obj_t *det = lv_obj_create(left);
    lv_obj_set_size(det, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(det);
    lv_obj_set_layout(det, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(det, det_cols, det_rows);
    lv_obj_set_style_pad_gap(det, 10, 0);

    _make_detail_cell(det, 0, 0, "HUMIDITY",   &s_lbl_humidity);
    _make_detail_cell(det, 1, 0, "WIND",       &s_lbl_wind);
    _make_detail_cell(det, 0, 1, "PRESSURE",   &s_lbl_pressure);
    _make_detail_cell(det, 1, 1, "FEELS LIKE", &s_lbl_feels);

    /* ── Right panel ─────────────────────────────────────────────────── */
    lv_obj_t *right = lv_obj_create(parent);
    lv_obj_set_size(right, LV_PCT(50), LV_PCT(100));
    lv_obj_set_pos(right, LV_PCT(50), 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right, 0, 0);
    lv_obj_set_style_pad_hor(right, 44, 0);
    lv_obj_set_style_pad_ver(right, 36, 0);
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(right, 32, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Daily forecast ──────────────────────────────────────────────── */
    lv_obj_t *daily_sec = lv_obj_create(right);
    lv_obj_set_size(daily_sec, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(daily_sec);
    lv_obj_set_layout(daily_sec, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(daily_sec, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(daily_sec, 14, 0);

    _section_heading(daily_sec, "3-DAY FORECAST");

    static lv_coord_t fcst_cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_SENTINEL};
    static lv_coord_t fcst_rows[] = {LV_SIZE_CONTENT, LV_GRID_SENTINEL};

    lv_obj_t *daily_grid = lv_obj_create(daily_sec);
    lv_obj_set_size(daily_grid, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(daily_grid);
    lv_obj_set_layout(daily_grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(daily_grid, fcst_cols, fcst_rows);
    lv_obj_set_style_pad_gap(daily_grid, 12, 0);

    for (int i = 0; i < DAILY_COUNT; i++) {
        _make_fcst_card(daily_grid, i,
                        &s_daily_day[i], &s_daily_icon[i],
                        &s_daily_high[i], &s_daily_low[i]);
    }

    /* ── Hourly forecast ─────────────────────────────────────────────── */
    lv_obj_t *hourly_sec = lv_obj_create(right);
    lv_obj_set_size(hourly_sec, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(hourly_sec);
    lv_obj_set_layout(hourly_sec, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hourly_sec, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(hourly_sec, 14, 0);

    _section_heading(hourly_sec, "NEXT 3 HOURS");

    lv_obj_t *hourly_grid = lv_obj_create(hourly_sec);
    lv_obj_set_size(hourly_grid, LV_PCT(100), LV_SIZE_CONTENT);
    ui_style_ghost(hourly_grid);
    lv_obj_set_layout(hourly_grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(hourly_grid, fcst_cols, fcst_rows);
    lv_obj_set_style_pad_gap(hourly_grid, 12, 0);

    for (int i = 0; i < HOURLY_COUNT; i++) {
        lv_obj_t *dummy1, *dummy2;
        _make_fcst_card(hourly_grid, i,
                        &s_hourly_hour[i], &s_hourly_icon[i],
                        &s_hourly_temp[i], &dummy1);
        lv_obj_del(dummy2 = dummy1); /* no low-temp row for hourly — delete sub label */
        (void)dummy2;
    }

    /* Start 1-second tick */
    lv_timer_create(_clock_tick, 1000, NULL);
    _clock_tick(NULL);
}

/* ══════════════════════════════════════════════════════════════════════
 * State update functions
 * ══════════════════════════════════════════════════════════════════════ */
void ui_update_weather(const ui_weather_t *w) {
    lv_label_set_text_fmt(s_lbl_temp, "%.1f%s", w->temp, w->unit);
    lv_label_set_text(s_lbl_cond, w->cond);
    lv_label_set_text_fmt(s_lbl_humidity, "%.0f%%", w->humidity);
    lv_label_set_text_fmt(s_lbl_wind, "%.1f km/h", w->wind_speed);
    lv_label_set_text_fmt(s_lbl_pressure, "%.0f hPa", w->pressure);
    lv_label_set_text_fmt(s_lbl_feels, "%.1f%s", w->feels_like, w->unit);
}

void ui_update_forecast(const ui_day_forecast_t *days, int count) {
    int n = count < DAILY_COUNT ? count : DAILY_COUNT;
    for (int i = 0; i < n; i++) {
        lv_label_set_text(s_daily_day[i],  days[i].day);
        lv_label_set_text(s_daily_icon[i], _cond_icon(days[i].cond));
        lv_label_set_text_fmt(s_daily_high[i], "%.0f°", days[i].high);
        lv_label_set_text_fmt(s_daily_low[i],  "%.0f°", days[i].low);
    }
}

void ui_update_forecast_hourly(const ui_hour_forecast_t *hours, int count) {
    int n = count < HOURLY_COUNT ? count : HOURLY_COUNT;
    for (int i = 0; i < n; i++) {
        lv_label_set_text(s_hourly_hour[i], hours[i].hour);
        lv_label_set_text(s_hourly_icon[i], _cond_icon(hours[i].cond));
        lv_label_set_text_fmt(s_hourly_temp[i], "%.0f°", hours[i].temp);
    }
}
