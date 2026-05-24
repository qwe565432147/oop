#pragma once
#include "light.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Light     base;
    int       pin;
    bool      active_high;
    bool      state;
} LED;

/* 初始化（不分配内存，传入已存在的 LED 对象指针） */
void led_init(LED *self, int pin, bool active_high);
void led_deinit(LED *self);

#ifdef __cplusplus
}
#endif
