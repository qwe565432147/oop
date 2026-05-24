#include "led.h"
#include "esp_log.h"

static const char *TAG = "LED";

static void led_set_on(Light *base) {
    LED *self = (LED *)base;
    self->state = true;
    int level = self->active_high ? 1 : 0;
    ESP_LOGI(TAG, "GPIO_%d -> %d (ON)", self->pin, level);
}

static void led_set_off(Light *base) {
    LED *self = (LED *)base;
    self->state = false;
    int level = self->active_high ? 0 : 1;
    ESP_LOGI(TAG, "GPIO_%d -> %d (OFF)", self->pin, level);
}

static void led_toggle(Light *base) {
    LED *self = (LED *)base;
    if (self->state)
        led_set_off(base);
    else
        led_set_on(base);
}

static void led_set_brightness(Light *base, uint8_t brightness) {
    LED *self = (LED *)base;
    self->state = (brightness > 0);
    int level = self->active_high ? 1 : 0;
    ESP_LOGI(TAG, "GPIO_%d PWM=%u (level %d)%s", self->pin, brightness, level,
             self->state ? "" : " OFF");
}

static bool led_is_on(Light *base) {
    LED *self = (LED *)base;
    return self->state;
}

static const LightOps led_ops = {
    .set_on         = led_set_on,
    .set_off        = led_set_off,
    .toggle         = led_toggle,
    .set_brightness = led_set_brightness,
    .is_on          = led_is_on,
};

void led_init(LED *self, int pin, bool active_high) {
    self->base.ops    = &led_ops;
    self->pin         = pin;
    self->active_high = active_high;
    self->state       = false;
}

void led_deinit(LED *self) {
    led_set_off(&self->base);
    ESP_LOGI(TAG, "GPIO_%d deinit", self->pin);
}
