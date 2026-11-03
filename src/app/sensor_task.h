// Background task (core 0): IMU sampling, gyro calibration, sensor fusion and touch polling.
#pragma once

namespace sensor_task {
// Initialises the IMU + touch hardware and starts the task. Returns false if the IMU is missing.
bool start();
} // namespace sensor_task
