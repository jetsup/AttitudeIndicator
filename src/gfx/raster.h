// Software rasteriser that writes directly into the LVGL partial render buffer.
// All coordinates are absolute screen pixels; every call clips itself to the current strip.
#pragma once
#include <lvgl.h>

namespace raster {

inline uint16_t rgb565(uint32_t c) {
    return ((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x001F);
}

// Sky/ground fill split by an anti-aliased 2 px white horizon line, clipped to the round panel.
// (hx,hy) is a point on the horizon, (nx,ny) the unit normal pointing towards the ground.
void background(lv_layer_t* l, float hx, float hy, float nx, float ny, uint32_t sky_rgb, uint32_t ground_rgb);

// Anti-aliased line segment with round ends.
void segment(lv_layer_t* l, float x1, float y1, float x2, float y2, float width, uint32_t rgb);

// 5x7 bitmap digit (0-3) with its top-left corner at (x0, y0).
void digit(lv_layer_t* l, int d, int x0, int y0, uint32_t rgb);

// Solid downward-pointing triangle with its base on screen row 0, centred on x = 120.
void top_pointer(lv_layer_t* l, uint32_t rgb);

} // namespace raster
