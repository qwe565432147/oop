#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Light Light;

typedef struct {
    void     (*set_on)(Light *self);
    void     (*set_off)(Light *self);
    void     (*toggle)(Light *self);
    void     (*set_brightness)(Light *self, uint8_t brightness);
    bool     (*is_on)(Light *self);
} LightOps;

struct Light {
    const LightOps *ops;
};

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
