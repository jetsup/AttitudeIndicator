// Attitude indicator for the Waveshare ESP32-S3-Touch-LCD-1.28.
// Wiring only: sensor task (app/) feeds attitude_state, attitude_view (gfx/) renders it through LVGL.
#include <Arduino.h>
#include "app/attitude_state.h"
#include "app/sensor_task.h"
#include "drivers/gc9a01.h"
#include "gfx/attitude_view.h"
#include "gfx/lvgl_port.h"

// LVGL's recursive renderer needs far more than the default 8 KB Arduino loop stack.
SET_LOOP_TASK_STACK_SIZE(24 * 1024);

void setup() {
    Serial.begin(115200);
#ifdef SIM_ATTITUDE
    bool imu_ok = false;
#else
    bool imu_ok = sensor_task::start();
#endif
    Serial.printf("IMU %s\n", imu_ok ? "ok" : "NOT FOUND");

    lvgl_port::init();
    attitude_view::create(lv_screen_active());
    lvgl_port::run_once(); // render the first frame before lighting the backlight
    gc9a01::backlight(true);
}

void loop() {
    lvgl_port::run_once();

    static uint32_t last_print = 0, last_frames = 0;
    uint32_t now = millis();
    if (now - last_print >= 1000) {
        uint32_t frames = lvgl_port::frame_count();
        auto a = attitude_state::get_attitude();
        Serial.printf("%lu fps | pitch %6.1f roll %6.1f | heap %u\n",
                      (unsigned long)((frames - last_frames) * 1000 / (now - last_print)), a.pitch_deg, a.roll_deg,
                      (unsigned)ESP.getFreeHeap());
        last_frames = frames;
        last_print = now;
    }
    delay(1);
}
