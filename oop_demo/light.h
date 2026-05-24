#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
 * ============================================================
 *  抽象基类 —— 灯接口
 *  所有灯都继承 Light，通过 LightOps 虚表实现多态。
 * ============================================================
 */

#ifdef __cplusplus
extern "C" {
#endif

/* 前置声明 */
typedef struct Light Light;

/*
 * LightOps：虚函数表
 * 每个具体灯类型提供一个 const 实例，包含所有操作的具体实现。
 * 运行时通过 Light::ops 指针找到正确的函数调用 —— 这就是多态。
 */
typedef struct {
    void     (*set_on)(Light *self);
    void     (*set_off)(Light *self);
    void     (*toggle)(Light *self);
    void     (*set_brightness)(Light *self, uint8_t brightness);
    bool     (*is_on)(Light *self);
} LightOps;

/*
 * Light：抽象基类
 * ops 指针指向具体类型的虚表，实现运行期多态分发。
 * 派生类（LED、RGBLight）的第一个成员必须是 Light base，
 * 这样 (Light *)&derived 转换才合法。
 */
struct Light {
    const LightOps *ops;
};

/* ---- 多态接口函数（inline 封装） ---- */
/* 这些是推荐的通用调用方式：Light *p = ...; light_on(p); */

static inline void light_on(Light *self) {
    self->ops->set_on(self);
}

static inline void light_off(Light *self) {
    self->ops->set_off(self);
}

static inline void light_toggle(Light *self) {
    self->ops->toggle(self);
}

static inline void light_set_brightness(Light *self, uint8_t brightness) {
    self->ops->set_brightness(self, brightness);
}

static inline bool light_is_on(Light *self) {
    return self->ops->is_on(self);
}

#ifdef __cplusplus
}
#endif
