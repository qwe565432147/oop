#pragma once
#include "light.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Light     base;
    uint8_t   addr;
    uint8_t   r, g, b;
    bool      state;
} RGBLight;

void rgb_light_init(RGBLight *self, uint8_t addr);
void rgb_light_deinit(RGBLight *self);
void rgb_light_set_color(RGBLight *self, uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif
