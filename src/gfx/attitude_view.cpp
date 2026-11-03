#include "gfx/attitude_view.h"
#include <Arduino.h>
#include <math.h>
#include "app/attitude_state.h"
#include "board_config.h"
#include "gfx/raster.h"

namespace attitude_view {
namespace {

constexpr uint32_t SKY = 0x2B7CD3, GROUND = 0x8A5A2B, WHITE = 0xFFFFFF, AIRCRAFT = 0xFFC107;

lv_obj_t *canvas = nullptr;
lv_obj_t *toast = nullptr;
attitude_state::Attitude g_att = {0, 0};     // latest attitude, sampled once per frame by refresh()

// Draws the instrument straight into the render buffer (see gfx/raster.h).
void on_draw(lv_event_t *e) {
    lv_layer_t *l = lv_event_get_layer(e);
    const float CX = 120, CY = 120, PPD = 2.4f;     // pixels per degree of pitch
    const float r = g_att.roll_deg * 0.0174532925f;
    const float cr = cosf(r), sr = sinf(r);
    // horizon direction (right) and "towards ground" normal in screen coordinates
    const float dx = cr, dy = -sr, nx = sr, ny = cr;
    const float hx = CX + nx * g_att.pitch_deg * PPD, hy = CY + ny * g_att.pitch_deg * PPD;

    raster::background(l, hx, hy, nx, ny, SKY, GROUND);

    // pitch ladder (marks in degrees above the horizon)
    for (int p = -30; p <= 30; p += 5) {
        if (p == 0) continue;
        float off = -p * PPD;
        float mx = hx + nx * off, my = hy + ny * off;
        if (fabsf(mx - CX) + fabsf(my - CY) > 150) continue;
        bool major = (p % 10) == 0;
        float half = major ? 26 : 14;
        raster::segment(l, mx - dx * half, my - dy * half, mx + dx * half, my + dy * half, major ? 2 : 1, WHITE);
        if (major) {
            int n = abs(p) / 10;                        // 1, 2, 3 -> "10", "20", "30"
            for (int side = -1; side <= 1; side += 2) {
                float lx = mx + dx * side * (half + 14), ly = my + dy * side * (half + 14);
                raster::digit(l, n, (int)lx - 6, (int)ly - 3, WHITE);
                raster::digit(l, 0, (int)lx + 1, (int)ly - 3, WHITE);
            }
        }
    }

    // bank scale: ticks rotate with the sky, fixed pointer at the top
    static const int ticks[] = {-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60};
    for (int t : ticks) {
        float a = (t - g_att.roll_deg) * 0.0174532925f;      // screen angle clockwise from top
        float sa = sinf(a), ca = cosf(a);
        float r1 = 104, r2 = (t % 30 == 0) ? 94 : 98;
        if (t == 0) r2 = 92;
        raster::segment(l, CX + sa * r1, CY - ca * r1, CX + sa * r2, CY - ca * r2, (t % 30 == 0) ? 3 : 2, WHITE);
    }
    raster::top_pointer(l, AIRCRAFT);

    // fixed aircraft symbol
    raster::segment(l, CX - 70, CY, CX - 26, CY, 5, AIRCRAFT);
    raster::segment(l, CX + 26, CY, CX + 70, CY, 5, AIRCRAFT);
    raster::segment(l, CX - 26, CY, CX - 26, CY + 10, 5, AIRCRAFT);
    raster::segment(l, CX + 26, CY, CX + 26, CY + 10, 5, AIRCRAFT);
    raster::segment(l, CX, CY, CX, CY, 8, AIRCRAFT);   // centre dot
}

void refresh(lv_timer_t *) {
    g_att = attitude_state::get_attitude();
    lv_obj_invalidate(canvas);

    uint32_t age = attitude_state::ms_since_mode_change();
    if (age < 1500) {
        lv_label_set_text(toast, attitude_state::get_mode() == attitude_state::MODE_UPRIGHT ? "UPRIGHT" : "FLAT");
        lv_obj_remove_flag(toast, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(toast, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace

void create(lv_obj_t *screen) {
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    canvas = lv_obj_create(screen);
    lv_obj_remove_style_all(canvas);
    lv_obj_set_size(canvas, board::LCD_W, board::LCD_H);
    lv_obj_set_pos(canvas, 0, 0);
    lv_obj_add_event_cb(canvas, on_draw, LV_EVENT_DRAW_MAIN, nullptr);

    toast = lv_label_create(screen);
    lv_obj_set_style_text_color(toast, lv_color_white(), 0);
    lv_obj_set_style_text_font(toast, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_color(toast, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(toast, LV_OPA_70, 0);
    lv_obj_set_style_pad_all(toast, 5, 0);
    lv_obj_set_style_radius(toast, 6, 0);
    lv_obj_align(toast, LV_ALIGN_CENTER, 0, 62);
    lv_obj_add_flag(toast, LV_OBJ_FLAG_HIDDEN);

    lv_timer_create(refresh, 1, nullptr);
}

}  // namespace attitude_view
