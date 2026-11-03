// Waveshare ESP32-S3-Touch-LCD-1.28: pin map and panel geometry.
#pragma once

namespace board {
// GC9A01 round LCD (SPI2)
constexpr int LCD_DC = 8, LCD_CS = 9, LCD_SCK = 10, LCD_MOSI = 11, LCD_RST = 14, LCD_BL = 2;
constexpr int LCD_W = 240, LCD_H = 240;
// Shared I2C bus: QMI8658 IMU + CST816S touch
constexpr int I2C_SDA = 6, I2C_SCL = 7, TP_RST = 13;
}  // namespace board
