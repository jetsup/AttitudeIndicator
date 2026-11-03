// GC9A01 240x240 round LCD driver: ESP-IDF SPI master + DMA, asynchronous pixel flush.
#pragma once
#include <stdint.h>

namespace gc9a01 {

constexpr int MAX_FLUSH_PIXELS = 240 * 68;   // one SPI transaction is limited to 32 KB

void init();
void backlight(bool on);

// Start a DMA transfer of big-endian RGB565 pixels for the given window. Returns immediately.
void flush(int x1, int y1, int x2, int y2, const uint8_t *px);

// True exactly once after each flush() has fully completed (safe to poll from task context).
bool take_flush_done();

}  // namespace gc9a01
