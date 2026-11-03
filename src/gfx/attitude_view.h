// The attitude-indicator screen: horizon, pitch ladder, bank scale, aircraft symbol, mode toast.
#pragma once
#include <lvgl.h>

namespace attitude_view {
void create(lv_obj_t* screen); // builds the widgets and the per-frame refresh timer
}
