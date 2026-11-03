#include "app/sensor_task.h"
#include <Arduino.h>
#include "app/attitude_state.h"
#include "drivers/cst816.h"
#include "drivers/i2c_bus.h"
#include "drivers/qmi8658.h"
#include "fusion/ahrs.h"

namespace sensor_task {
namespace {

constexpr float D2R = 0.0174532925f;

// QMI8658 frame -> body frame (x: screen top, y: screen left, z: out of screen).
// Determined empirically on this board; it is a proper rotation (180 deg about the Y axis).
struct V3 {
    float x, y, z;
};
inline V3 to_body(float sx, float sy, float sz) {
    return {-sx, sy, -sz};
}

void calibrate(Ahrs& ahrs) {
    qmi8658::Sample s;
    V3 g_sum{0, 0, 0}, a_sum{0, 0, 0};
    int n = 0;
    uint32_t t0 = millis();
    while (n < 250 && millis() - t0 < 4000) {
        if (qmi8658::read(s)) {
            V3 g = to_body(s.gx, s.gy, s.gz), a = to_body(s.ax, s.ay, s.az);
            g_sum.x += g.x;
            g_sum.y += g.y;
            g_sum.z += g.z;
            a_sum.x += a.x;
            a_sum.y += a.y;
            a_sum.z += a.z;
            n++;
        }
        vTaskDelay(1);
    }
    if (n <= 20)
        return;
    g_sum.x /= n;
    g_sum.y /= n;
    g_sum.z /= n;
    if (fabsf(g_sum.x) < 20 && fabsf(g_sum.y) < 20 && fabsf(g_sum.z) < 20) // ignore if it was moving
        ahrs.setGyroBias(g_sum.x * D2R, g_sum.y * D2R, g_sum.z * D2R);
    ahrs.initFromAccel(a_sum.x / n, a_sum.y / n, a_sum.z / n);
}

void run(void*) {
    Ahrs ahrs;
    calibrate(ahrs);

    qmi8658::Sample s;
    uint32_t last = micros();
    TickType_t wake = xTaskGetTickCount();
    uint32_t tick = 0;
    bool was_touching = false;
    for (;;) {
        vTaskDelayUntil(&wake, 2); // poll ~500 Hz, sensor produces 250 Hz
        if (qmi8658::read(s)) {
            uint32_t now = micros();
            float dt = (now - last) * 1e-6f;
            last = now;
            if (dt > 0.05f)
                dt = 0.05f;
            V3 g = to_body(s.gx, s.gy, s.gz), a = to_body(s.ax, s.ay, s.az);
            ahrs.update(g.x * D2R, g.y * D2R, g.z * D2R, a.x, a.y, a.z, dt);
            float ux, uy, uz;
            ahrs.up(ux, uy, uz);
            attitude_state::publish_up_vector(ux, uy, uz);
        }
        if ((++tick % 10) == 0) { // a tap toggles the mounting mode
            bool t = cst816::touching();
            if (t && !was_touching)
                attitude_state::toggle_mode();
            was_touching = t;
        }
    }
}

} // namespace

bool start() {
    i2c_bus::begin();
    if (!qmi8658::begin())
        return false;
    cst816::begin();
    xTaskCreatePinnedToCore(run, "sensor", 12288, nullptr, 5, nullptr, 0);
    return true;
}

} // namespace sensor_task
