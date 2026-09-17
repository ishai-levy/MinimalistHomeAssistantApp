/* ui.h — Main header for the smart-home dashboard LVGL UI
 * Target: Guition JC8012P4A1C  (ESP32-P4 + ESP32-C6)
 * Display: 10.1" IPS 800×1280, used landscape (1280×800)
 * Framework: LVGL v8.3, ESP-IDF or Arduino
 *
 * Usage:
 *   1. Call ui_init() once after lv_init() and display driver setup.
 *   2. Call the ui_update_*() functions from your MQTT receive callbacks.
 *   3. Drive lv_task_handler() at ≥ 5 ms intervals from your main loop.
 */
#pragma once
#include "lvgl.h"
#include "ui_colors.h"
#include "ui_fonts.h"
#include <stdbool.h>
#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════════════
 * State structs — mirror the MQTT JSON payload shapes from the Python
 * backend (dashboard/state/* topics).
 * ═══════════════════════════════════════════════════════════════════════ */

typedef struct {
    float temp;
    char  unit[8];    /* "°C" or "°F" */
    char  cond[64];   /* "clear", "cloudy", "rain", … */
    float humidity;
    float wind_speed;
    float pressure;
    float feels_like;
} ui_weather_t;

typedef struct {
    char  day[12];    /* "Mon", "Tue", … */
    float high;
    float low;
    char  cond[64];
} ui_day_forecast_t;

typedef struct {
    char  hour[8];    /* "14:00" */
    float temp;
    char  cond[64];
} ui_hour_forecast_t;

typedef struct {
    bool  playing;
    char  title[128];
    char  artist[128];
    char  album[128];
    char  art_url[256];   /* MA HTTP image URL – fetch async, load via lv_img */
    char  uri[256];       /* track URI for favorite_add/remove */
    int   position_ms;
    int   duration_ms;
    int   volume;         /* 0–100 */
    bool  shuffle;
} ui_now_playing_t;

typedef struct {
    char title[128];
    char artist[128];
    char uri[256];
} ui_queue_item_t;

typedef struct {
    char id[64];
    char kind[8];         /* "alarm" | "timer" */
    double trigger_at;    /* Unix epoch float */
    char player_id[64];
} ui_schedule_entry_t;

typedef struct {
    char id[64];
    char kind[8];         /* "alarm" | "timer" */
    char player_id[64];
} ui_ringing_entry_t;

typedef struct {
    char player_id[64];
    char name[64];
} ui_player_t;

typedef struct {
    char name[128];
    char uri[256];
} ui_playlist_t;

/* ═══════════════════════════════════════════════════════════════════════
 * Init
 * ═══════════════════════════════════════════════════════════════════════ */

/* Call once after lv_init() and display driver registration. */
void ui_init(void);

/* ═══════════════════════════════════════════════════════════════════════
 * State-update API — call from MQTT/HTTP callbacks (thread-safe note:
 * if your MQTT thread is separate, wrap calls with lv_port_mutex_lock).
 * ═══════════════════════════════════════════════════════════════════════ */

void ui_update_weather(const ui_weather_t *w);
void ui_update_forecast(const ui_day_forecast_t *days, int count);
void ui_update_forecast_hourly(const ui_hour_forecast_t *hours, int count);

void ui_update_dude(bool on);
void ui_update_vacuum(const char *state);

void ui_update_now_playing(const ui_now_playing_t *np);
void ui_update_queue_next(const ui_queue_item_t *items, int count);
void ui_update_shuffle(bool on);

void ui_update_alarms(const ui_schedule_entry_t *entries, int count);
void ui_update_timers(const ui_schedule_entry_t *entries, int count);
void ui_update_ringing(const ui_ringing_entry_t *entries, int count);

/* Called from HTTP /players and /playlists responses */
void ui_update_players(const ui_player_t *players, int count);
void ui_update_playlists(const ui_playlist_t *playlists, int count);

/* ═══════════════════════════════════════════════════════════════════════
 * Overlay helpers (used internally and by ui_home.c)
 * ═══════════════════════════════════════════════════════════════════════ */

void ui_overlay_show_alarm_modal(void);

/* ═══════════════════════════════════════════════════════════════════════
 * Internal tab builders (called only by ui_init)
 * ═══════════════════════════════════════════════════════════════════════ */

void ui_clock_build(lv_obj_t *parent);
void ui_home_build(lv_obj_t *parent);
void ui_music_build(lv_obj_t *parent);

/* ═══════════════════════════════════════════════════════════════════════
 * Shared style helpers (used across tab files)
 * ═══════════════════════════════════════════════════════════════════════ */

/* Apply surface-2 card look: bg, border, radius 16, no padding */
void ui_style_card(lv_obj_t *obj);

/* Remove bg, border, padding from a container */
void ui_style_ghost(lv_obj_t *obj);

/* Configure obj as a scrollable flex column that respects its bounds */
void ui_style_scroll_col(lv_obj_t *obj);
