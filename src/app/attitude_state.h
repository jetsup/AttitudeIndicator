// Thread-safe hand-off between the sensor task (producer) and the UI (consumer).
#pragma once
#include <stdint.h>

namespace attitude_state {

enum Mode { MODE_FLAT = 0, MODE_UPRIGHT = 1 }; // device lying on a desk / held like a panel instrument

struct Attitude {
    float pitch_deg;
    float roll_deg;
}; // nose-up and right-bank positive

// producer side
void publish_up_vector(float x, float y, float z); // gravity "up" direction in the body frame
void toggle_mode();

// consumer side
Attitude get_attitude();
Mode get_mode();
uint32_t ms_since_mode_change(); // UINT32_MAX if never changed

} // namespace attitude_state
