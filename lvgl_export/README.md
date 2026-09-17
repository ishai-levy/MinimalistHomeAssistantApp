# Smart-Home Dashboard — LVGL C Export

Target: **Guition JC8012P4A1C** (ESP32-P4 @ 400 MHz + ESP32-C6 Wi-Fi)  
Display: 10.1" IPS 800×1280, used **landscape → 1280×800**  
Framework: **LVGL v8.3** on ESP-IDF or Arduino-ESP32

---

## File map

| File | Purpose |
|------|---------|
| `ui_colors.h` | Color token macros (exact hex from CSS variables) |
| `ui_fonts.h` | Font size aliases + `lv_font_conv` compilation commands |
| `ui.h` | Structs, public API declarations |
| `ui.c` | `ui_init()`, custom pill tab bar, shared style helpers |
| `ui_clock.c` | Clock & Weather tab, 1-second timer, forecast cards |
| `ui_home.c` | Home tab: 3×2 grid, vacuum, dude toggle, timer arc, alarm card |
| `ui_music.c` | Music tab: 500×500 art, transport, seek/volume bars, queue/playlists |
| `ui_overlay.c` | Alarm modal (HH:MM spinners), ringing popup (STOP) |

---

## Quick start

### 1. Add files to your project

In `CMakeLists.txt` / `idf_component_register`:

```cmake
idf_component_register(
  SRCS
    "ui.c"
    "ui_clock.c"
    "ui_home.c"
    "ui_music.c"
    "ui_overlay.c"
    # add compiled font .c files here after step 2
  INCLUDE_DIRS "."
)
```

### 2. Compile custom fonts

See the detailed instructions inside `ui_fonts.h`.  
TL;DR:

```bash
npm install -g lv_font_conv
# then run the lv_font_conv commands in ui_fonts.h
```

Then edit `ui_fonts.h`: uncomment the `LV_FONT_DECLARE` lines and change the
`#define UI_FONT_*` macros from `&lv_font_montserrat_*` to `&ui_font_*`.

### 3. Enable built-in Montserrat fonts (interim fallback)

In `menuconfig → LVGL → Fonts`:

```
CONFIG_LV_FONT_MONTSERRAT_10=y
CONFIG_LV_FONT_MONTSERRAT_12=y
CONFIG_LV_FONT_MONTSERRAT_14=y
CONFIG_LV_FONT_MONTSERRAT_16=y
CONFIG_LV_FONT_MONTSERRAT_18=y
CONFIG_LV_FONT_MONTSERRAT_20=y
CONFIG_LV_FONT_MONTSERRAT_32=y
CONFIG_LV_FONT_MONTSERRAT_48=y
```

### 4. Initialize in your `app_main`

```c
#include "ui.h"

void app_main(void) {
    // ... display & touch driver init ...
    lv_init();
    // ... register display & input device drivers ...
    ui_init();   // <-- call this

    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

### 5. Wire MQTT state updates

Subscribe to `dashboard/state/#` and call the matching update function:

```c
// In your MQTT event handler (guard with lv_port_mutex_lock if separate thread):
if (strcmp(topic, "dashboard/state/weather") == 0) {
    ui_weather_t w = parse_weather_json(payload);
    ui_update_weather(&w);
}
if (strcmp(topic, "dashboard/state/now-playing") == 0) {
    ui_now_playing_t np = parse_now_playing_json(payload);
    ui_update_now_playing(&np);
}
if (strcmp(topic, "dashboard/state/ringing") == 0) {
    ui_ringing_entry_t entries[4];
    int n = parse_ringing_json(payload, entries, 4);
    ui_update_ringing(entries, n);
}
// ... etc for alarms, timers, vacuum, dude, shuffle, queue_next ...
```

### 6. Wire HTTP /players and /playlists

Fetch these once on startup (and on reconnect):

```c
// GET http://brain:PORT/players  → parse JSON array → call:
ui_update_players(players, n);

// GET http://brain:PORT/playlists → parse JSON array → call:
ui_update_playlists(playlists, n);
```

---

## Album art

`ui_update_now_playing()` receives `np->art_url`.  
The placeholder in the art box will remain until you:

1. Fetch the image bytes over HTTP into PSRAM.
2. Decode to RGB (use `libjpeg` or `lvgl/src/libs/libjpeg`).
3. Call `lv_img_set_src(s_art_img, &dsc)` with an `lv_img_dsc_t` pointing at the buffer.

---

## TODO markers in the code

Search for `// TODO:` — every MQTT publish and HTTP call is marked there with the exact topic/command string from the Python backend.

---

## Known limitations / next steps

- **Font sizes**: The 140 px clock font is stubbed to Montserrat 48 until compiled.
- **Arc label position**: `lv_obj_align_to` places the timer label at card-build time; call it again after any arc resize if needed.
- **Thread safety**: If MQTT runs in a FreeRTOS task, wrap all `ui_update_*` calls with `lvgl_port_lock(0)` / `lvgl_port_unlock()` (from `esp_lvgl_port`).
- **Image loading**: Art URL fetch is a stub; implement async HTTP → PSRAM → `lv_img`.
