// Thin register-level helpers for the shared I2C bus.
#pragma once
#include <Arduino.h>

namespace i2c_bus {
void begin();
bool write8(uint8_t addr, uint8_t reg, uint8_t val);
bool read(uint8_t addr, uint8_t reg, uint8_t *buf, size_t n);
}  // namespace i2c_bus
