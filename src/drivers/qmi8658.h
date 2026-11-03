// QMI8658 6-axis IMU driver (accel +-4 g, gyro +-512 dps, 250 Hz).
#pragma once
#include <Arduino.h>

namespace qmi8658 {

struct Sample {
    float ax, ay, az, gx, gy, gz;
}; // g and deg/s, in the sensor's own frame

bool begin();
bool read(Sample& s); // true if a fresh accel+gyro sample was read

} // namespace qmi8658
