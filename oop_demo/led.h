#pragma once
#include "light.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 前置类型声明 ---- */
typedef struct LED LED;

/*
 * fn 方法表（per-instance，支持 Python 风格调用）
 *
 * 每个 LED 对象内部都存有一份函数指针表，
 * 调用时通过 led.fn.set_on(&led) 直达对应实现。
 * 代价：每个实例多占 ~5×sizeof(void*) 字节。
 *
 * 与 LightOps 的区别：
 *   LightOps → 放在常量区，所有同类实例共享，用于多态
 *   LED_Fn   → 放在实例内，每个实例独有，用于 obj.fn.method(obj) 风格
 */
typedef struct {
    void (*set_on)(LED *self);
    void (*set_off)(LED *self);
    void (*toggle)(LED *self);
    void (*set_brightness)(LED *self, uint8_t brightness);
    bool (*is_on)(LED *self);
} LED_Fn;

struct LED {
    /* 继承：首成员必须是 Light，才能安全转型为 Light* */
    Light     base;

    /* fn 方法表：实现对象.方法() 风格调用 */
    LED_Fn    fn;

    /* LED 特有属性 */
    int       pin;          /* GPIO 引脚号 */
    bool      active_high;  /* true=高电平点亮，false=低电平点亮 */
    bool      state;        /* 当前开关状态 */
};

/* ---- 构造函数（不分配内存，只初始化） ---- */
/* 使用方式：
 *   static LED led;            // 静态分配
 *   led_init(&led, 13, true);  // 初始化
 *   led.fn.set_on(&led);       // 对象.方法 风格
 */
void led_init(LED *self, int pin, bool active_high);
void led_deinit(LED *self);

#ifdef __cplusplus
}
#endif
