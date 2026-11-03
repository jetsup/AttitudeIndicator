#include "gfx/raster.h"
#include <math.h>
#include "board_config.h"

namespace raster {
namespace {

static inline uint16_t mix565(uint32_t a, uint32_t b, int t) {   // t: 0..256 -> a..b (24-bit colours)
    int r = (((a >> 16) & 255) * (256 - t) + ((b >> 16) & 255) * t) >> 8;
    int g = (((a >> 8) & 255) * (256 - t) + ((b >> 8) & 255) * t) >> 8;
    int bl = ((a & 255) * (256 - t) + (b & 255) * t) >> 8;
    return rgb565((r << 16) | (g << 8) | bl);
}

static inline uint16_t blend565(uint16_t dst, uint16_t src, int c) {   // c: 0..256 (src weight)
    int r = (((dst >> 11) & 31) * (256 - c) + ((src >> 11) & 31) * c) >> 8;
    int g = (((dst >> 5) & 63) * (256 - c) + ((src >> 5) & 63) * c) >> 8;
    int b = ((dst & 31) * (256 - c) + (src & 31) * c) >> 8;
    return (r << 11) | (g << 5) | b;
}

}  // namespace

void background(lv_layer_t *l, float hx, float hy, float nx, float ny, uint32_t sky_rgb, uint32_t ground_rgb) {
    lv_draw_buf_t *db = l->draw_buf;
    const int stride = db->header.stride;
    const uint16_t sky = rgb565(sky_rgb), gnd = rgb565(ground_rgb), white = 0xFFFF;
    const float R = 119.5f;
    const float anx = fabsf(nx);
    for (int y = l->buf_area.y1; y <= l->buf_area.y2; y++) {
        uint16_t *row = (uint16_t *)((uint8_t *)db->data + (y - l->buf_area.y1) * stride) - l->buf_area.x1;
        float dyc = y - R;
        float hw2 = R * R - dyc * dyc;
        if (hw2 < 0) { for (int x = l->buf_area.x1; x <= l->buf_area.x2; x++) row[x] = 0; continue; }
        float hw = sqrtf(hw2);
        int xs = (int)ceilf(R - hw), xe = (int)floorf(R + hw);
        for (int x = l->buf_area.x1; x < xs; x++) row[x] = 0;
        for (int x = xe + 1; x <= l->buf_area.x2; x++) row[x] = 0;
        if (xs < l->buf_area.x1) xs = l->buf_area.x1;
        if (xe > l->buf_area.x2) xe = l->buf_area.x2;
        // signed distance to the horizon: d(x) = d0 + (x + 0.5) * nx  (positive = ground)
        float d0 = (0.5f - hx) * nx + (y + 0.5f - hy) * ny;
        int band0, band1;       // pixels in [band0, band1) need per-pixel work (|d| < 2); always within [xs, xe+1]
        if (anx < 1e-4f) {
            if (fabsf(d0) < 2.0f) { band0 = xs; band1 = xe + 1; } else { band0 = band1 = xe + 1; }
        } else {
            // clamp in the float domain first: the horizon crossing can be far outside the screen
            const float lo = (float)xs, hi = (float)(xe + 1);
            float xc = -d0 / nx;
            float w = 2.0f / anx + 1.0f;
            float f0 = floorf(xc - w), f1 = ceilf(xc + w);
            band0 = (int)(f0 < lo ? lo : (f0 > hi ? hi : f0));
            band1 = (int)(f1 < lo ? lo : (f1 > hi ? hi : f1));
            if (band1 < band0) band1 = band0;
        }
        bool groundLeft = (d0 + xs * nx) > 0;
        bool groundRight = (d0 + xe * nx) > 0;
        if (anx < 1e-4f) groundLeft = groundRight = d0 > 0;
        for (int x = xs; x < band0; x++) row[x] = groundLeft ? gnd : sky;
        for (int x = band1; x <= xe; x++) row[x] = groundRight ? gnd : sky;
        for (int x = band0; x < band1; x++) {
            float d = d0 + x * nx;
            int t = (int)((d + 0.5f) * 256.0f);
            if (t < 0) t = 0; if (t > 256) t = 256;
            uint16_t px = (t == 0) ? sky : (t == 256) ? gnd : mix565(sky_rgb, ground_rgb, t);
            float cw = 1.5f - fabsf(d);                  // 2 px wide white horizon line
            if (cw > 0) px = blend565(px, white, cw >= 1.0f ? 256 : (int)(cw * 256));
            row[x] = px;
        }
    }
}

// Anti-aliased line segment with round ends, rasterised straight into the render buffer.
void segment(lv_layer_t *l, float x1, float y1, float x2, float y2, float width, uint32_t rgb) {
    const float r = width * 0.5f + 0.5f;                 // coverage reaches 0 at distance r
    const float minx = (x1 < x2 ? x1 : x2) - r - 1, maxx = (x1 > x2 ? x1 : x2) + r + 1;
    const float miny = (y1 < y2 ? y1 : y2) - r - 1, maxy = (y1 > y2 ? y1 : y2) + r + 1;
    int ya = (int)miny, yb = (int)maxy + 1;
    if (ya < l->buf_area.y1) ya = l->buf_area.y1;
    if (yb > l->buf_area.y2) yb = l->buf_area.y2;
    if (ya > yb) return;                                  // not in this strip
    int bxa = (int)minx - 1, bxb = (int)maxx + 1;
    if (bxa < 0) bxa = 0;
    if (bxb > 239) bxb = 239;
    lv_draw_buf_t *db = l->draw_buf;
    const int stride = db->header.stride;
    const uint16_t col = rgb565(rgb);
    const float dx = x2 - x1, dy = y2 - y1;
    const float len2 = dx * dx + dy * dy;
    const float len = sqrtf(len2);
    const float ux = len > 1e-3f ? dx / len : 1, uy = len > 1e-3f ? dy / len : 0;
    const float a = -uy, invA = fabsf(a) > 1e-4f ? 1.0f / a : 0;
    const float r2 = r * r;
    for (int y = ya; y <= yb; y++) {
        uint16_t *row = (uint16_t *)((uint8_t *)db->data + (y - l->buf_area.y1) * stride) - l->buf_area.x1;
        const float wy = y + 0.5f - y1;
        // x-interval of this row that can touch the capsule: hull of the slab and both end-cap discs
        float lo = 1e9f, hi = -1e9f;
        const float b = (0.5f - x1) * a + wy * ux;        // perpendicular distance pd(x) = a*x + b
        if (invA == 0) {
            if (fabsf(b) <= r) { lo = minx; hi = maxx; }
        } else {
            float p = (-r - b) * invA, q = (r - b) * invA;
            if (p > q) { float tmp = p; p = q; q = tmp; }
            lo = p; hi = q;
        }
        float e1 = r2 - wy * wy;
        if (e1 > 0) { float h = sqrtf(e1); if (x1 - h < lo) lo = x1 - h; if (x1 + h > hi) hi = x1 + h; }
        const float wy2 = y + 0.5f - y2;
        float e2 = r2 - wy2 * wy2;
        if (e2 > 0) { float h = sqrtf(e2); if (x2 - h < lo) lo = x2 - h; if (x2 + h > hi) hi = x2 + h; }
        if (hi < lo) continue;
        int xa = (int)(lo - 1.0f), xb = (int)(hi + 2.0f);   // coordinates are positive on screen
        if (lo < 1.0f) xa = 0;
        if (xa < bxa) xa = bxa;
        if (xb > bxb) xb = bxb;
        for (int x = xa; x <= xb; x++) {
            const float wx = x + 0.5f - x1;
            const float t = wx * ux + wy * uy;
            float dist;
            if (t < 0) dist = sqrtf(wx * wx + wy * wy);
            else if (t > len) { float ex = x + 0.5f - x2, ey = y + 0.5f - y2; dist = sqrtf(ex * ex + ey * ey); }
            else dist = fabsf(wx * a + wy * ux);
            const float cov = r - dist;
            if (cov <= 0) continue;
            row[x] = cov >= 1.0f ? col : blend565(row[x], col, (int)(cov * 256));
        }
    }
}

// 5x7 bitmap digits for the pitch-ladder numbers.
static const uint8_t DIGITS[4][7] = {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},   // 0
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},   // 1
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},   // 2
    {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E},   // 3
};
void digit(lv_layer_t *l, int d, int x0, int y0, uint32_t rgb) {
    const uint16_t col = rgb565(rgb);
    lv_draw_buf_t *db = l->draw_buf;
    const int stride = db->header.stride;
    for (int r = 0; r < 7; r++) {
        int y = y0 + r;
        if (y < l->buf_area.y1 || y > l->buf_area.y2) continue;
        uint16_t *row = (uint16_t *)((uint8_t *)db->data + (y - l->buf_area.y1) * stride) - l->buf_area.x1;
        uint8_t bits = DIGITS[d][r];
        for (int c = 0; c < 5; c++)
            if ((bits >> (4 - c)) & 1) { int x = x0 + c; if (x >= 0 && x < 240) row[x] = col; }
    }
}

// Solid downward-pointing bank pointer at the top of the dial (rows 0..13).
void top_pointer(lv_layer_t *l, uint32_t rgb) {
    lv_draw_buf_t *db = l->draw_buf;
    const int stride = db->header.stride;
    const uint16_t col = rgb565(rgb);
    for (int y = 0; y < 15; y++) {
        if (y < l->buf_area.y1 || y > l->buf_area.y2) continue;
        uint16_t *row = (uint16_t *)((uint8_t *)db->data + (y - l->buf_area.y1) * stride) - l->buf_area.x1;
        float hw = 8.0f * (15 - y) / 15.0f;
        int xa = (int)ceilf(120 - hw), xb = (int)floorf(119 + hw);
        for (int x = xa; x <= xb; x++) row[x] = col;
    }
}

}  // namespace raster
