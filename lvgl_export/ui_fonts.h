/* ui_fonts.h — Font declarations for the dashboard UI
 *
 * Custom fonts must be compiled with lv_font_conv before use.
 * Until then, built-in Montserrat sizes are used as fallbacks.
 *
 * ── How to compile custom fonts ───────────────────────────────────────
 *
 * 1. Install lv_font_conv:
 *      npm install -g lv_font_conv
 *
 * 2. Download font files:
 *    - JetBrains Mono: https://fonts.google.com/specimen/JetBrains+Mono
 *      (JetBrainsMono-Light.ttf, JetBrainsMono-Regular.ttf)
 *    - Outfit:         https://fonts.google.com/specimen/Outfit
 *      (Outfit-Regular.ttf, Outfit-Medium.ttf, Outfit-SemiBold.ttf)
 *
 * 3. Run in the lvgl_export/ directory:
 *
 *   JetBrains Mono:
 *     lv_font_conv --font JetBrainsMono-Light.ttf   --range 0x20-0x7E --size 140 --bpp 4 --format lvgl -o ui_font_mono_140.c --no-compress
 *     lv_font_conv --font JetBrainsMono-Light.ttf   --range 0x20-0x7E --size 48  --bpp 4 --format lvgl -o ui_font_mono_48.c  --no-compress
 *     lv_font_conv --font JetBrainsMono-Regular.ttf --range 0x20-0x7E --size 32  --bpp 4 --format lvgl -o ui_font_mono_32.c  --no-compress
 *     lv_font_conv --font JetBrainsMono-Regular.ttf --range 0x20-0x7E --size 18  --bpp 4 --format lvgl -o ui_font_mono_18.c  --no-compress
 *     lv_font_conv --font JetBrainsMono-Regular.ttf --range 0x20-0x7E --size 14  --bpp 4 --format lvgl -o ui_font_mono_14.c  --no-compress
 *     lv_font_conv --font JetBrainsMono-Regular.ttf --range 0x20-0x7E --size 12  --bpp 4 --format lvgl -o ui_font_mono_12.c  --no-compress
 *     lv_font_conv --font JetBrainsMono-Regular.ttf --range 0x20-0x7E --size 11  --bpp 4 --format lvgl -o ui_font_mono_11.c  --no-compress
 *     lv_font_conv --font JetBrainsMono-Regular.ttf --range 0x20-0x7E --size 10  --bpp 4 --format lvgl -o ui_font_mono_10.c  --no-compress
 *
 *   Outfit:
 *     lv_font_conv --font Outfit-SemiBold.ttf --range 0x20-0x7E --size 20 --bpp 4 --format lvgl -o ui_font_ui_20.c --no-compress
 *     lv_font_conv --font Outfit-SemiBold.ttf --range 0x20-0x7E --size 16 --bpp 4 --format lvgl -o ui_font_ui_16.c --no-compress
 *     lv_font_conv --font Outfit-Medium.ttf   --range 0x20-0x7E --size 14 --bpp 4 --format lvgl -o ui_font_ui_14.c --no-compress
 *     lv_font_conv --font Outfit-Regular.ttf  --range 0x20-0x7E --size 13 --bpp 4 --format lvgl -o ui_font_ui_13.c --no-compress
 *     lv_font_conv --font Outfit-Regular.ttf  --range 0x20-0x7E --size 12 --bpp 4 --format lvgl -o ui_font_ui_12.c --no-compress
 *     lv_font_conv --font Outfit-Regular.ttf  --range 0x20-0x7E --size 11 --bpp 4 --format lvgl -o ui_font_ui_11.c --no-compress
 *
 * 4. Add the generated .c files to your CMakeLists.txt / idf_component_register SRCS.
 *
 * 5. Uncomment the LV_FONT_DECLARE lines below and switch the #defines to point
 *    at the custom font structs instead of lv_font_montserrat_*.
 *
 * ── Enable CONFIG_LV_FONT_MONTSERRAT_* in menuconfig for fallbacks ────
 *   CONFIG_LV_FONT_MONTSERRAT_10=y
 *   CONFIG_LV_FONT_MONTSERRAT_12=y
 *   CONFIG_LV_FONT_MONTSERRAT_14=y
 *   CONFIG_LV_FONT_MONTSERRAT_16=y
 *   CONFIG_LV_FONT_MONTSERRAT_18=y
 *   CONFIG_LV_FONT_MONTSERRAT_20=y
 *   CONFIG_LV_FONT_MONTSERRAT_24=y
 *   CONFIG_LV_FONT_MONTSERRAT_28=y
 *   CONFIG_LV_FONT_MONTSERRAT_32=y
 *   CONFIG_LV_FONT_MONTSERRAT_36=y
 *   CONFIG_LV_FONT_MONTSERRAT_48=y
 */
#pragma once
#include "lvgl.h"

/* ── Uncomment after compiling custom fonts ───────────────────────────── */
// LV_FONT_DECLARE(ui_font_mono_140)
// LV_FONT_DECLARE(ui_font_mono_48)
// LV_FONT_DECLARE(ui_font_mono_32)
// LV_FONT_DECLARE(ui_font_mono_18)
// LV_FONT_DECLARE(ui_font_mono_14)
// LV_FONT_DECLARE(ui_font_mono_12)
// LV_FONT_DECLARE(ui_font_mono_11)
// LV_FONT_DECLARE(ui_font_mono_10)
// LV_FONT_DECLARE(ui_font_ui_20)
// LV_FONT_DECLARE(ui_font_ui_16)
// LV_FONT_DECLARE(ui_font_ui_14)
// LV_FONT_DECLARE(ui_font_ui_13)
// LV_FONT_DECLARE(ui_font_ui_12)
// LV_FONT_DECLARE(ui_font_ui_11)

/* ── Fallbacks: built-in Montserrat (replace after compiling) ─────────── */
#define UI_FONT_MONO_140    (&lv_font_montserrat_48)   /* REPLACE: &ui_font_mono_140 */
#define UI_FONT_MONO_48     (&lv_font_montserrat_48)   /* REPLACE: &ui_font_mono_48  */
#define UI_FONT_MONO_32     (&lv_font_montserrat_32)   /* REPLACE: &ui_font_mono_32  */
#define UI_FONT_MONO_18     (&lv_font_montserrat_18)   /* REPLACE: &ui_font_mono_18  */
#define UI_FONT_MONO_14     (&lv_font_montserrat_14)   /* REPLACE: &ui_font_mono_14  */
#define UI_FONT_MONO_12     (&lv_font_montserrat_12)   /* REPLACE: &ui_font_mono_12  */
#define UI_FONT_MONO_11     (&lv_font_montserrat_10)   /* REPLACE: &ui_font_mono_11  */
#define UI_FONT_MONO_10     (&lv_font_montserrat_10)   /* REPLACE: &ui_font_mono_10  */
#define UI_FONT_UI_20       (&lv_font_montserrat_20)   /* REPLACE: &ui_font_ui_20    */
#define UI_FONT_UI_16       (&lv_font_montserrat_16)   /* REPLACE: &ui_font_ui_16    */
#define UI_FONT_UI_14       (&lv_font_montserrat_14)   /* REPLACE: &ui_font_ui_14    */
#define UI_FONT_UI_13       (&lv_font_montserrat_12)   /* REPLACE: &ui_font_ui_13    */
#define UI_FONT_UI_12       (&lv_font_montserrat_12)   /* REPLACE: &ui_font_ui_12    */
#define UI_FONT_UI_11       (&lv_font_montserrat_10)   /* REPLACE: &ui_font_ui_11    */
