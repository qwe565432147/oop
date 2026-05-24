/******************************************************************************
 *  oop_demo — C语言面向对象风格灯光模块使用示例
 *
 *  演示两种调用风格：
 *    1. Python风格：led.fn.set_on(&led)     —— 对象.方法(对象)
 *    2. 多态风格：  light_on(lights[i])      —— 统一接口，无视具体类型
 ******************************************************************************/

#include <stdio.h>
#include "led.h"
#include "rgb_light.h"

int main(void)
{
    printf("========== 1. 创建灯对象（静态分配 + init） ==========\n\n");

    /* 静态分配（零 malloc，适合任何 MCU） */
    static LED      led_status;     /* .bss 段 */
    static LED      led_warning;
    static RGBLight rgb_strip;

    /* 初始化：分配和初始化分离 */
    led_init(&led_status,  13, true);   /* GPIO13, 高电平点亮 */
    led_init(&led_warning, 12, false);  /* GPIO12, 低电平点亮 */
    rgb_light_init(&rgb_strip, 0x22);   /* I2C 地址 0x22 */


    printf("========== 2. Python 风格调用：对象.方法(对象) ==========\n\n");

    /* 直接通过对象的 fn 方法表调用 —— 像 Python 一样直观 */
    printf("--- 开灯 ---\n");
    led_status.fn.set_on(&led_status);
    led_warning.fn.set_on(&led_warning);
    rgb_strip.fn.set_on(&rgb_strip);

    printf("\n--- 设置 RGB 颜色（特有操作） ---\n");
    rgb_light_set_color(&rgb_strip, 255, 0, 0);

    printf("\n--- 关灯 ---\n");
    led_status.fn.set_off(&led_status);

    printf("\n--- 翻转 ---\n");
    rgb_strip.fn.toggle(&rgb_strip);
    rgb_strip.fn.toggle(&rgb_strip);

    printf("\n--- 调亮度 ---\n");
    led_warning.fn.set_brightness(&led_warning, 128);

    printf("\n--- 查状态 ---\n");
    printf("led_status  is %s\n",  led_status.fn.is_on(&led_status)  ? "ON" : "OFF");
    printf("led_warning is %s\n",  led_warning.fn.is_on(&led_warning) ? "ON" : "OFF");
    printf("rgb_strip   is %s\n",  rgb_strip.fn.is_on(&rgb_strip)   ? "ON" : "OFF");


    printf("\n========== 3. 多态风格调用：统一接口操作不同类型 ==========\n\n");

    /*
     * 多态：所有灯都可以转型为 Light*，
     * 通过 light_on/off/toggle 等接口统一操作，
     * 运行期自动派发到正确的实现。
     */
    Light *lights[] = {
        (Light *)&led_status,
        (Light *)&led_warning,
        (Light *)&rgb_strip,
    };

    for (int i = 0; i < 3; i++) {
        printf("light_on(lights[%d]) → ", i);
        light_on(lights[i]);
    }

    for (int i = 0; i < 3; i++) {
        printf("light_is_on(lights[%d]) → %s\n",
               i, light_is_on(lights[i]) ? "ON" : "OFF");
    }


    printf("\n========== 4. 反初始化 ==========\n\n");
    led_deinit(&led_status);
    led_deinit(&led_warning);
    rgb_light_deinit(&rgb_strip);

    return 0;
}
