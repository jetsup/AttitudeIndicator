#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (96 * 1024U)

#define LV_DEF_REFR_PERIOD 10
#define LV_USE_FLOAT 1          /* sub-pixel coordinates => smooth motion */
#define LV_USE_OS LV_OS_NONE
#define LV_USE_LOG 0
#define LV_USE_PERF_MONITOR 0

#define LV_DRAW_SW_COMPLEX 1    /* anti-aliased lines / polygons */
#define LV_DRAW_SW_SHADOW_CACHE_SIZE 0

#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_12

#endif
