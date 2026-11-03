#include "drivers/i2c_bus.h"
#include <Wire.h>
#include "board_config.h"

namespace i2c_bus {

void begin() {
    Wire.begin(board::I2C_SDA, board::I2C_SCL, 400000);
    Wire.setTimeOut(5);
}

bool write8(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

bool read(uint8_t addr, uint8_t reg, uint8_t* buf, size_t n) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
        return false;
    if (Wire.requestFrom((int)addr, (int)n) != (int)n)
        return false;
    for (size_t i = 0; i < n; i++)
        buf[i] = Wire.read();
    return true;
}

} // namespace i2c_bus
