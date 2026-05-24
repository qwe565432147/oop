#include "rgb_light.h"
#include <stdio.h>

/* ---- 内部实现 ---- */

static void rgb_send(RGBLight *self) {
    /* 模拟通过 I2C 协议发送颜色数据 */
    printf("[RGB] I2C: addr=0x%02X -> R=%u G=%u B=%u\n",
           self->addr, self->r, self->g, self->b);
}

static void rgb_set_on(RGBLight *self) {
    self->state = true;
    rgb_send(self);
}

static void rgb_set_off(RGBLight *self) {
    self->state = false;
    printf("[RGB] I2C: addr=0x%02X -> R=0 G=0 B=0 (OFF)\n", self->addr);
}

static void rgb_toggle(RGBLight *self) {
    if (self->state)
        rgb_set_off(self);
    else
        rgb_set_on(self);
}

static void rgb_set_brightness(RGBLight *self, uint8_t brightness) {
    self->state = (brightness > 0);
    uint8_t r = (uint16_t)self->r * brightness / 255;
    uint8_t g = (uint16_t)self->g * brightness / 255;
    uint8_t b = (uint16_t)self->b * brightness / 255;
    printf("[RGB] I2C: addr=0x%02X brightness=%u -> scaled(%u,%u,%u)%s\n",
           self->addr, brightness, r, g, b,
           self->state ? "" : " OFF");
}

static bool rgb_is_on(RGBLight *self) {
    return self->state;
}

/* ---- 多态适配 ---- */

static void rgb_ops_set_on(Light *base) {
    rgb_set_on((RGBLight *)base);
}

static void rgb_ops_set_off(Light *base) {
    rgb_set_off((RGBLight *)base);
}

static void rgb_ops_toggle(Light *base) {
    rgb_toggle((RGBLight *)base);
}

static void rgb_ops_set_brightness(Light *base, uint8_t brightness) {
    rgb_set_brightness((RGBLight *)base, brightness);
}

static bool rgb_ops_is_on(Light *base) {
    return rgb_is_on((RGBLight *)base);
}

/* ---- 虚表 ---- */
static const LightOps rgb_ops = {
    .set_on         = rgb_ops_set_on,
    .set_off        = rgb_ops_set_off,
    .toggle         = rgb_ops_toggle,
    .set_brightness = rgb_ops_set_brightness,
    .is_on          = rgb_ops_is_on,
};

/* ---- 初始化 & 反初始化 ---- */

void rgb_light_init(RGBLight *self, uint8_t addr) {
    self->base.ops = &rgb_ops;

    self->fn.set_on         = rgb_set_on;
    self->fn.set_off        = rgb_set_off;
    self->fn.toggle         = rgb_toggle;
    self->fn.set_brightness = rgb_set_brightness;
    self->fn.is_on          = rgb_is_on;

    self->addr     = addr;
    self->r = self->g = self->b = 255;
    self->state    = false;
}

void rgb_light_deinit(RGBLight *self) {
    rgb_set_off(self);
    printf("[RGB] addr=0x%02X deinit\n", self->addr);
}

void rgb_light_set_color(RGBLight *self, uint8_t r, uint8_t g, uint8_t b) {
    self->r = r;
    self->g = g;
    self->b = b;
    printf("[RGB] I2C: addr=0x%02X color set (%u,%u,%u)\n",
           self->addr, r, g, b);
}
