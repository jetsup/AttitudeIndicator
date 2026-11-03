#include "drivers/qmi8658.h"
#include "drivers/i2c_bus.h"

namespace qmi8658 {
namespace {
constexpr float ACC_LSB_PER_G = 8192.0f;      // +-4 g
constexpr float GYR_LSB_PER_DPS = 64.0f;      // +-512 dps
uint8_t addr = 0x6B;
}  // namespace

bool begin() {
    bool found = false;
    for (uint8_t a : {0x6B, 0x6A}) {
        uint8_t id = 0;
        if (i2c_bus::read(a, 0x00, &id, 1) && id == 0x05) { addr = a; found = true; break; }
    }
    if (!found) return false;
    i2c_bus::write8(addr, 0x60, 0xB0);         // soft reset
    delay(30);
    i2c_bus::write8(addr, 0x02, 0x40);         // register auto-increment
    i2c_bus::write8(addr, 0x03, 0x15);         // accel: +-4 g, 250 Hz
    i2c_bus::write8(addr, 0x04, 0x55);         // gyro: +-512 dps, 250 Hz
    i2c_bus::write8(addr, 0x06, 0x11);         // low-pass filters on
    i2c_bus::write8(addr, 0x08, 0x03);         // enable accel + gyro
    delay(20);
    return true;
}

bool read(Sample &s) {
    uint8_t b[19];                              // STATUS0 (0x2E) .. GZ_H (0x40)
    if (!i2c_bus::read(addr, 0x2E, b, sizeof(b))) return false;
    if ((b[0] & 0x03) != 0x03) return false;
    auto i16 = [&](int o) { return (int16_t)(b[o] | (b[o + 1] << 8)); };
    s.ax = i16(7) / ACC_LSB_PER_G;    s.ay = i16(9) / ACC_LSB_PER_G;    s.az = i16(11) / ACC_LSB_PER_G;
    s.gx = i16(13) / GYR_LSB_PER_DPS; s.gy = i16(15) / GYR_LSB_PER_DPS; s.gz = i16(17) / GYR_LSB_PER_DPS;
    return true;
}

}  // namespace qmi8658
