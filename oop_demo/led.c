#include "led.h"
#include <stdio.h>

/*
 * ---- 内部实现函数 ----
 * 这些 static 函数真正完成硬件的读写操作。
 * 它们不对外暴露，用户只能通过 fn 方法表或 LightOps 调用。
 */

static void led_set_on(LED *self) {
    self->state = true;
    int level = self->active_high ? 1 : 0;
    printf("[LED] GPIO_%d -> %d (ON)\n", self->pin, level);
}

static void led_set_off(LED *self) {
    self->state = false;
    int level = self->active_high ? 0 : 1;
    printf("[LED] GPIO_%d -> %d (OFF)\n", self->pin, level);
}

static void led_toggle(LED *self) {
    if (self->state)
        led_set_off(self);
    else
        led_set_on(self);
}

static void led_set_brightness(LED *self, uint8_t brightness) {
    self->state = (brightness > 0);
    int level = self->active_high ? 1 : 0;
    printf("[LED] GPIO_%d PWM=%u (level %d)%s\n",
           self->pin, brightness, level,
           self->state ? "" : " OFF");
}

static bool led_is_on(LED *self) {
    return self->state;
}

/* ---- 多态适配：将具体类型函数包装成 LightOps 签名 ---- */
/* LightOps 的函数签名是 void (*)(Light*)，需要把 Light* 转型回 LED* */

static void led_ops_set_on(Light *base) {
    led_set_on((LED *)base);
}

static void led_ops_set_off(Light *base) {
    led_set_off((LED *)base);
}

static void led_ops_toggle(Light *base) {
    led_toggle((LED *)base);
}

static void led_ops_set_brightness(Light *base, uint8_t brightness) {
    led_set_brightness((LED *)base, brightness);
}

static bool led_ops_is_on(Light *base) {
    return led_is_on((LED *)base);
}

/* ---- 虚表（所有 LED 实例共享，放在常量区） ---- */
static const LightOps led_ops = {
    .set_on         = led_ops_set_on,
    .set_off        = led_ops_set_off,
    .toggle         = led_ops_toggle,
    .set_brightness = led_ops_set_brightness,
    .is_on          = led_ops_is_on,
};

/* ---- 初始化 & 反初始化 ---- */

void led_init(LED *self, int pin, bool active_high) {
    /* 1. 绑定虚表（供多态使用） */
    self->base.ops = &led_ops;

    /* 2. 绑定 fn 方法表（供 Python 风格使用） */
    self->fn.set_on         = led_set_on;
    self->fn.set_off        = led_set_off;
    self->fn.toggle         = led_toggle;
    self->fn.set_brightness = led_set_brightness;
    self->fn.is_on          = led_is_on;

    /* 3. 初始化属性 */
    self->pin         = pin;
    self->active_high = active_high;
    self->state       = false;
}

void led_deinit(LED *self) {
    led_set_off(self);
    printf("[LED] GPIO_%d deinit\n", self->pin);
}
