// LVGL <-> GC9A01 glue: tick source, partial-render DMA buffers, flush callback.
#pragma once
#include <lvgl.h>

namespace lvgl_port {

lv_display_t* init();   // brings up LVGL and the LCD (backlight stays off until first frame is ready)
void run_once();        // call from the main loop: completes finished flushes, runs LVGL timers
uint32_t frame_count(); // number of fully rendered frames

} // namespace lvgl_port
