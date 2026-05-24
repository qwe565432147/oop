#include <stdio.h>
#include "led.h"
#include "rgb_light.h"

/* 静态分配灯对象 — 零 malloc，适合任何 MCU */
static LED      led_status;      /* .bss 段 */
static LED      led_warning;
static RGBLight rgb_strip;

void app_main(void)
{
    /* 初始化：传入已分配好的对象地址 */
    led_init(&led_status,  13, true);   /* GPIO13，高电平点亮 */
    led_init(&led_warning, 12, false);  /* GPIO12，低电平点亮 */
    rgb_light_init(&rgb_strip, 0x22);   /* I2C 地址 0x22 */

    /* 通过统一接口操作 — 无视底层差异 */
    Light *lights[] = {
        (Light *)&led_status,
        (Light *)&led_warning,
        (Light *)&rgb_strip,
    };

    for (int i = 0; i < 3; i++)
        light_on(lights[i]);

    /* RGB 灯设置颜色（特有接口，向下转型） */
    rgb_light_set_color(&rgb_strip, 255, 0, 0);

    light_toggle(lights[2]);
    light_toggle(lights[2]);

    light_set_brightness(lights[1], 128);

    for (int i = 0; i < 3; i++)
        printf("lights[%d] is %s\n", i, light_is_on(lights[i]) ? "ON" : "OFF");

    /* 反初始化 */
    led_deinit(&led_status);
    led_deinit(&led_warning);
    rgb_light_deinit(&rgb_strip);
}
