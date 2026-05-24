#pragma once
#include "light.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RGBLight RGBLight;

/*
 * RGBLight 的 fn 方法表
 * 注意函数指针的第一个参数是 RGBLight*，与 LED_Fn 不同。
 * 这就是为什么每个类型需要自己的方法表类型定义。
 */
typedef struct {
    void (*set_on)(RGBLight *self);
    void (*set_off)(RGBLight *self);
    void (*toggle)(RGBLight *self);
    void (*set_brightness)(RGBLight *self, uint8_t brightness);
    bool (*is_on)(RGBLight *self);
} RGBLight_Fn;

struct RGBLight {
    Light        base;     /* 继承基类 */
    RGBLight_Fn  fn;       /* 对象.方法() 风格 */
    uint8_t      addr;     /* I2C 从机地址 */
    uint8_t      r, g, b;  /* 当前颜色 */
    bool         state;    /* 当前开关状态 */
};

void rgb_light_init(RGBLight *self, uint8_t addr);
void rgb_light_deinit(RGBLight *self);

/* RGBLight 的特有操作（不在基类接口中） */
void rgb_light_set_color(RGBLight *self, uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif
