#include "drivers/cst816.h"
#include <Arduino.h>
#include "board_config.h"
#include "drivers/i2c_bus.h"

namespace cst816 {
namespace { constexpr uint8_t ADDR = 0x15; }

void begin() {
    pinMode(board::TP_RST, OUTPUT);
    digitalWrite(board::TP_RST, LOW);  delay(10);
    digitalWrite(board::TP_RST, HIGH); delay(60);
    i2c_bus::write8(ADDR, 0xFE, 0x01);         // disable auto standby
}

bool touching() {
    uint8_t b[3];
    return i2c_bus::read(ADDR, 0x01, b, 3) && (b[1] & 0x0F) > 0;
}

}  // namespace cst816
