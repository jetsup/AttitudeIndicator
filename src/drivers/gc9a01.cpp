#include "drivers/gc9a01.h"
#include <Arduino.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "board_config.h"

namespace gc9a01 {
namespace {

using namespace board;
enum { USR_CMD = 0, USR_DATA = 1, USR_PIXELS = 3 }; // bit0: DC level, bit1: flush-complete marker

spi_device_handle_t dev;
spi_transaction_t pix_trans;
volatile bool inflight = false;
volatile bool flush_done = false;

void IRAM_ATTR pre_cb(spi_transaction_t* t) {
    gpio_set_level((gpio_num_t)LCD_DC, ((intptr_t)t->user) & 1);
}
// ISR context: only raise a flag. LVGL is notified later from task context.
void IRAM_ATTR post_cb(spi_transaction_t* t) {
    if (((intptr_t)t->user) & 2)
        flush_done = true;
}

void send(uint8_t v, bool data) {
    spi_transaction_t t = {};
    t.length = 8;
    t.flags = SPI_TRANS_USE_TXDATA;
    t.tx_data[0] = v;
    t.user = (void*)(intptr_t)(data ? USR_DATA : USR_CMD);
    spi_device_polling_transmit(dev, &t);
}
void cmd(uint8_t c) {
    send(c, false);
}
void cmd(uint8_t c, std::initializer_list<uint8_t> d) {
    send(c, false);
    for (uint8_t b : d)
        send(b, true);
}

} // namespace

void init() {
    pinMode(LCD_DC, OUTPUT);
    pinMode(LCD_RST, OUTPUT);
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);

    spi_bus_config_t bus = {};
    bus.mosi_io_num = LCD_MOSI;
    bus.miso_io_num = -1;
    bus.sclk_io_num = LCD_SCK;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = MAX_FLUSH_PIXELS * 2;
    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t d = {};
    d.clock_speed_hz = 80 * 1000 * 1000;
    d.mode = 0;
    d.spics_io_num = LCD_CS;
    d.queue_size = 2;
    d.pre_cb = pre_cb;
    d.post_cb = post_cb;
    d.flags = SPI_DEVICE_NO_DUMMY;
    spi_bus_add_device(SPI2_HOST, &d, &dev);

    digitalWrite(LCD_RST, HIGH);
    delay(10);
    digitalWrite(LCD_RST, LOW);
    delay(10);
    digitalWrite(LCD_RST, HIGH);
    delay(120);

    cmd(0xEF);
    cmd(0xEB, {0x14});
    cmd(0xFE);
    cmd(0xEF);
    cmd(0xEB, {0x14});
    cmd(0x84, {0x40});
    cmd(0x85, {0xFF});
    cmd(0x86, {0xFF});
    cmd(0x87, {0xFF});
    cmd(0x88, {0x0A});
    cmd(0x89, {0x21});
    cmd(0x8A, {0x00});
    cmd(0x8B, {0x80});
    cmd(0x8C, {0x01});
    cmd(0x8D, {0x01});
    cmd(0x8E, {0xFF});
    cmd(0x8F, {0xFF});
    cmd(0xB6, {0x00, 0x20});
    cmd(0x36, {0x08}); // MADCTL: BGR
    cmd(0x3A, {0x05}); // 16 bpp
    cmd(0x90, {0x08, 0x08, 0x08, 0x08});
    cmd(0xBD, {0x06});
    cmd(0xBC, {0x00});
    cmd(0xFF, {0x60, 0x01, 0x04});
    cmd(0xC3, {0x13});
    cmd(0xC4, {0x13});
    cmd(0xC9, {0x22});
    cmd(0xBE, {0x11});
    cmd(0xE1, {0x10, 0x0E});
    cmd(0xDF, {0x21, 0x0C, 0x02});
    cmd(0xF0, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A});
    cmd(0xF1, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F});
    cmd(0xF2, {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A});
    cmd(0xF3, {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F});
    cmd(0xED, {0x1B, 0x0B});
    cmd(0xAE, {0x77});
    cmd(0xCD, {0x63});
    cmd(0x70, {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03});
    cmd(0xE8, {0x34});
    cmd(0x62, {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70});
    cmd(0x63, {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70});
    cmd(0x64, {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07});
    cmd(0x66, {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00});
    cmd(0x67, {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98});
    cmd(0x74, {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00});
    cmd(0x98, {0x3E, 0x07});
    cmd(0x35);
    cmd(0x21); // inversion on (panel needs it)
    cmd(0x11);
    delay(120);
    cmd(0x29);
    delay(20);
}

void backlight(bool on) {
    digitalWrite(LCD_BL, on);
}

void flush(int x1, int y1, int x2, int y2, const uint8_t* px) {
    if (inflight) { // collect the previous transaction's result
        spi_transaction_t* r;
        spi_device_get_trans_result(dev, &r, portMAX_DELAY);
        inflight = false;
    }
    cmd(0x2A, {(uint8_t)(x1 >> 8), (uint8_t)x1, (uint8_t)(x2 >> 8), (uint8_t)x2});
    cmd(0x2B, {(uint8_t)(y1 >> 8), (uint8_t)y1, (uint8_t)(y2 >> 8), (uint8_t)y2});
    cmd(0x2C);
    memset(&pix_trans, 0, sizeof(pix_trans));
    pix_trans.length = (size_t)(x2 - x1 + 1) * (y2 - y1 + 1) * 16;
    pix_trans.tx_buffer = px;
    pix_trans.user = (void*)(intptr_t)USR_PIXELS;
    flush_done = false;
    inflight = true;
    spi_device_queue_trans(dev, &pix_trans, portMAX_DELAY);
}

bool take_flush_done() {
    if (!flush_done)
        return false;
    flush_done = false;
    return true;
}

} // namespace gc9a01
