#include "rgb_light.h"
#include "esp_log.h"

static const char *TAG = "RGB";

static void rgb_send(RGBLight *self) {
    ESP_LOGI(TAG, "I2C: addr=0x%02X -> R=%u G=%u B=%u",
             self->addr, self->r, self->g, self->b);
}

static void rgb_set_on(Light *base) {
    RGBLight *self = (RGBLight *)base;
    self->state = true;
    rgb_send(self);
}

static void rgb_set_off(Light *base) {
    RGBLight *self = (RGBLight *)base;
    self->state = false;
    ESP_LOGI(TAG, "I2C: addr=0x%02X -> R=0 G=0 B=0 (OFF)", self->addr);
}

static void rgb_toggle(Light *base) {
    RGBLight *self = (RGBLight *)base;
    if (self->state)
        rgb_set_off(base);
    else
        rgb_set_on(base);
}

static void rgb_set_brightness(Light *base, uint8_t brightness) {
    RGBLight *self = (RGBLight *)base;
    self->state = (brightness > 0);
    uint8_t r = (uint16_t)self->r * brightness / 255;
    uint8_t g = (uint16_t)self->g * brightness / 255;
    uint8_t b = (uint16_t)self->b * brightness / 255;
    ESP_LOGI(TAG, "I2C: addr=0x%02X brightness=%u -> scaled(%u,%u,%u)%s",
             self->addr, brightness, r, g, b, self->state ? "" : " OFF");
}

static bool rgb_is_on(Light *base) {
    RGBLight *self = (RGBLight *)base;
    return self->state;
}

static const LightOps rgb_ops = {
    .set_on         = rgb_set_on,
    .set_off        = rgb_set_off,
    .toggle         = rgb_toggle,
    .set_brightness = rgb_set_brightness,
    .is_on          = rgb_is_on,
};

void rgb_light_init(RGBLight *self, uint8_t addr) {
    self->base.ops = &rgb_ops;
    self->addr     = addr;
    self->r = self->g = self->b = 255;
    self->state    = false;
}

void rgb_light_deinit(RGBLight *self) {
    rgb_set_off(&self->base);
    ESP_LOGI(TAG, "addr=0x%02X deinit", self->addr);
}

void rgb_light_set_color(RGBLight *self, uint8_t r, uint8_t g, uint8_t b) {
    self->r = r;
    self->g = g;
    self->b = b;
    ESP_LOGI(TAG, "I2C: addr=0x%02X color set (%u,%u,%u)", self->addr, r, g, b);
}
