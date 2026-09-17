/* ui_colors.h — Dashboard color palette for LVGL
 * Source: src/index.css CSS custom properties
 * Target: Guition JC8012P4A1C (ESP32-P4), 1280×800 landscape, LVGL v8.3+
 */
#pragma once
#include "lvgl.h"

/* ── Background layers ───────────────────────────────────────────────── */
#define UI_CLR_BG           lv_color_hex(0x07090e)
#define UI_CLR_SURF         lv_color_hex(0x0d1220)
#define UI_CLR_SURF2        lv_color_hex(0x141b2d)
#define UI_CLR_SURF3        lv_color_hex(0x1c2540)

/* ── Borders (rgba white composited onto UI_CLR_BG) ──────────────────── */
/* rgba(255,255,255,0.07) on #07090e  →  #181a1f  */
#define UI_CLR_BORDER       lv_color_hex(0x181a1f)
/* rgba(255,255,255,0.14) on #07090e  →  #2a2b30  */
#define UI_CLR_BORDER_BR    lv_color_hex(0x2a2b30)

/* ── Accent palette ──────────────────────────────────────────────────── */
#define UI_CLR_ACCENT       lv_color_hex(0x00c8f0)
#define UI_CLR_WARM         lv_color_hex(0xff7043)
#define UI_CLR_SUCCESS      lv_color_hex(0x26d07c)

/* ── Tinted surfaces (rgba accent composited onto UI_CLR_SURF2) ─────── */
/* rgba(0,200,240,0.12)   on #141b2d  →  #10303f  */
#define UI_CLR_ACCENT_BG    lv_color_hex(0x10303f)
/* rgba(255,112,67,0.12)  on #141b2d  →  #2e2120  */
#define UI_CLR_WARM_BG      lv_color_hex(0x2e2120)
/* rgba(38,208,124,0.12)  on #141b2d  →  #163229  */
#define UI_CLR_SUCCESS_BG   lv_color_hex(0x163229)

/* ── Text ────────────────────────────────────────────────────────────── */
#define UI_CLR_TEXT         lv_color_hex(0xdce6f5)
#define UI_CLR_TEXT2        lv_color_hex(0x8896b0)
#define UI_CLR_TEXT3        lv_color_hex(0x4a5568)
#define UI_CLR_MUTED        lv_color_hex(0x4a5568)

/* ── Opacity helpers ─────────────────────────────────────────────────── */
#define UI_OPA_MODAL        ((lv_opa_t)184)   /* 72% */
#define UI_OPA_RINGING      ((lv_opa_t)214)   /* 84% */
#define UI_OPA_VOL_IND      ((lv_opa_t)179)   /* 70% */
