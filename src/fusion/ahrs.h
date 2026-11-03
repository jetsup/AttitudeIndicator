// Mahony complementary filter (gyro + accelerometer) with startup gyro calibration.
// Body frame: x = towards screen top, y = towards screen left, z = out of the screen.
#pragma once
#include <math.h>

// Consumers should treat the filter as single-threaded state.

struct Ahrs {
    float q0 = 1, q1 = 0, q2 = 0, q3 = 0;
    float bx = 0, by = 0, bz = 0; // gyro bias (rad/s)
    float kp = 2.0f;

    void setGyroBias(float x, float y, float z) {
        bx = x;
        by = y;
        bz = z;
    }

    void initFromAccel(float ax, float ay, float az) {
        float n = sqrtf(ax * ax + ay * ay + az * az);
        if (n < 0.3f)
            return;
        ax /= n;
        ay /= n;
        az /= n;
        q0 = 1 + az;
        q1 = ay;
        q2 = -ax;
        q3 = 0;
        if (q0 < 1e-3f && fabsf(ax) < 1e-3f && fabsf(ay) < 1e-3f) {
            q0 = 0;
            q1 = 1;
            q2 = 0;
        }
        normalize();
    }

    void normalize() {
        float n = 1.0f / sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
        q0 *= n;
        q1 *= n;
        q2 *= n;
        q3 *= n;
    }

    // gyro in rad/s, accel in g, dt in s
    void update(float gx, float gy, float gz, float ax, float ay, float az, float dt) {
        gx -= bx;
        gy -= by;
        gz -= bz;
        float an = sqrtf(ax * ax + ay * ay + az * az);
        if (an > 0.3f) {
            // reduce accelerometer trust while the device is being accelerated
            float trust = 1.0f - fabsf(an - 1.0f) * 2.5f;
            if (trust < 0)
                trust = 0;
            float k = kp * trust;
            float inv = 1.0f / an;
            ax *= inv;
            ay *= inv;
            az *= inv;
            float vx = q1 * q3 - q0 * q2;
            float vy = q0 * q1 + q2 * q3;
            float vz = q0 * q0 - 0.5f + q3 * q3;
            gx += k * 2.0f * (ay * vz - az * vy);
            gy += k * 2.0f * (az * vx - ax * vz);
            gz += k * 2.0f * (ax * vy - ay * vx);
        }
        gx *= 0.5f * dt;
        gy *= 0.5f * dt;
        gz *= 0.5f * dt;
        float a = q0, b = q1, c = q2;
        q0 += -b * gx - c * gy - q3 * gz;
        q1 += a * gx + c * gz - q3 * gy;
        q2 += a * gy - b * gz + q3 * gx;
        q3 += a * gz + b * gy - c * gx;
        normalize();
    }

    // "Up" direction expressed in body axes
    void up(float& x, float& y, float& z) const {
        x = 2 * (q1 * q3 - q0 * q2);
        y = 2 * (q0 * q1 + q2 * q3);
        z = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;
    }
};
