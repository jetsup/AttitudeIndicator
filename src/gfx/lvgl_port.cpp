#include "gfx/lvgl_port.h"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include "board_config.h"
#include "drivers/gc9a01.h"

namespace lvgl_port {
namespace {

lv_display_t* disp = nullptr;
volatile uint32_t frames = 0;

// Tell LVGL a flush finished. Done from task context only: LVGL is not ISR-safe.
void complete_flush_if_ready() {
    if (gc9a01::take_flush_done())
        lv_display_flush_ready(disp);
}

void flush_cb(lv_display_t*, const lv_area_t* a, uint8_t* px) {
    uint32_t count = (a->x2 - a->x1 + 1) * (a->y2 - a->y1 + 1);
    lv_draw_rgb565_swap(px, count); // panel wants big-endian RGB565
    gc9a01::flush(a->x1, a->y1, a->x2, a->y2, px);
}

void flush_wait_cb(lv_display_t*) { // LVGL needs the buffer back: block until DMA is done
    while (!gc9a01::take_flush_done())
        taskYIELD();
    lv_display_flush_ready(disp);
}

void on_refresh_ready(lv_event_t*) {
    frames++;
}

} // namespace

lv_display_t* init() {
    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return millis(); });

    gc9a01::init();
    // LVGL aligns display buffers up to LV_DRAW_BUF_ALIGN (64) itself; give it that alignment plus slack
    // so the strip can never spill past the allocation and corrupt the heap.
    const size_t bytes = gc9a01::MAX_FLUSH_PIXELS * 2;
    const size_t alloc = bytes + 2 * LV_DRAW_BUF_ALIGN;
    uint8_t* buf1 = (uint8_t*)heap_caps_aligned_alloc(LV_DRAW_BUF_ALIGN, alloc, MALLOC_CAP_DMA);
    uint8_t* buf2 = (uint8_t*)heap_caps_aligned_alloc(LV_DRAW_BUF_ALIGN, alloc, MALLOC_CAP_DMA);
    if (!buf1 || !buf2) {
        Serial.println("FATAL: no DMA memory for render buffers");
        for (;;)
            delay(1000);
    }

    disp = lv_display_create(board::LCD_W, board::LCD_H);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, buf1, buf2, bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_flush_wait_cb(disp, flush_wait_cb);
    lv_display_add_event_cb(disp, on_refresh_ready, LV_EVENT_REFR_READY, nullptr);
    // Views paint their own pixels, so LVGL must not clear the buffer with a background rect.
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(lv_display_get_layer_bottom(disp), LV_OPA_TRANSP, 0);
    return disp;
}

void run_once() {
    complete_flush_if_ready();
    lv_timer_handler();
    complete_flush_if_ready();
}

uint32_t frame_count() {
    return frames;
}

} // namespace lvgl_port
