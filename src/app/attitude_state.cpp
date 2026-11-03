#include "app/attitude_state.h"
#include <Arduino.h>
#include <math.h>

namespace attitude_state {
namespace {
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
float up[3] = {0, 0, 1};
volatile Mode mode = MODE_FLAT;
volatile uint32_t mode_changed_at = 0;
volatile bool mode_changed = false;
constexpr float RAD2DEG = 57.29578f;
} // namespace

void publish_up_vector(float x, float y, float z) {
    portENTER_CRITICAL(&mux);
    up[0] = x;
    up[1] = y;
    up[2] = z;
    portEXIT_CRITICAL(&mux);
}

void toggle_mode() {
    mode = (mode == MODE_FLAT) ? MODE_UPRIGHT : MODE_FLAT;
    mode_changed_at = millis();
    mode_changed = true;
}

Mode get_mode() {
    return mode;
}

uint32_t ms_since_mode_change() {
    return mode_changed ? millis() - mode_changed_at : UINT32_MAX;
}

Attitude get_attitude() {
#ifdef SIM_ATTITUDE
    float t = millis() * 0.001f;
    return {75.0f * sinf(t * 0.7f), 170.0f * sinf(t * 0.45f)};
#endif
    float u[3];
    portENTER_CRITICAL(&mux);
    u[0] = up[0];
    u[1] = up[1];
    u[2] = up[2];
    portEXIT_CRITICAL(&mux);
    // Body frame: x = screen top, y = screen left, z = out of screen.
    // Express the aircraft axes (forward / right / up) in it.
    float fwd, right, upv;
    if (mode == MODE_FLAT) {
        fwd = u[0];
        right = -u[1];
        upv = u[2];
    } // nose = screen top
    else {
        fwd = -u[2];
        right = -u[1];
        upv = u[0];
    } // nose = away from viewer
    if (fwd > 1)
        fwd = 1;
    if (fwd < -1)
        fwd = -1;
    return {asinf(fwd) * RAD2DEG, atan2f(-right, upv) * RAD2DEG};
}

} // namespace attitude_state
