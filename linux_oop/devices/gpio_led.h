/*
 * gpio_led.h — GPIO 直控 LED 设备
 *
 * 这是第一个"具体设备类"，继承自 struct device。
 *
 * 继承方式：C 语言没有 extends 关键字，我们通过"嵌入"实现继承。
 *   struct gpio_led {
 *       struct device base;    ← 基类嵌入（相当于继承）
 *       int gpio_num;          ← 自己的属性
 *       ...
 *   };
 *
 * 因为 struct device 是第一个成员，所以：
 *   (struct device *)&gpio_led 是安全的（向上转型）
 *   container_of(dev, struct gpio_led, base) 恢复原类型（向下转型）
 *
 * 对比 Linux 内核：
 *   struct gpio_led  → 内核的 struct gpio_led (drivers/leds/leds-gpio.c)
 *   container_of     → 内核的 to_gpio_led() 宏
 *   .base.ops        → 继承自父类的操作表
 */

#ifndef __LINUX_OOP_GPIO_LED_H__
#define __LINUX_OOP_GPIO_LED_H__

#include "device.h"

/*
 * gpio_led — GPIO LED 设备
 *
 * 成员：
 *   base       — 继承自 device 的基类部分（必须放在第一位）
 *   gpio_num   — GPIO 引脚号
 *   active_high— true=高电平点亮，false=低电平点亮
 *   state      — 当前开关状态
 */
struct gpio_led {
	/* 继承：基类嵌入 */
	struct device	base;

	/* 自己的属性 */
	int		gpio_num;
	bool		active_high;
	bool		state;
};

/*
 * 为了方便向下转型，提供内联宏（内核风格的 to_xxx 函数）
 *
 * 用法：
 *   struct device *dev = device_find("led0");
 *   struct gpio_led *led = to_gpio_led(dev);
 */
static inline struct gpio_led *to_gpio_led(struct device *dev)
{
	return container_of(dev, struct gpio_led, base);
}

/* ================================================================
 *  公共接口
 *  ================================================================
 */

/**
 * gpio_led_create — 创建并初始化 GPIO LED 设备
 * @name:        设备名称
 * @gpio_num:    GPIO 引脚号
 * @active_high: 高/低电平有效
 * Return:       设备指针，失败返回 NULL
 *
 * 这是"构造函数"——分配 + 初始化 + 注册一步完成。
 * 实际内核驱动中，构造函数通常叫 probe。
 */
struct device *gpio_led_create(const char *name,
			       int gpio_num, bool active_high);

/**
 * gpio_led_set_on — 点亮 LED
 * @led: GPIO LED 设备（来自 to_gpio_led()）
 *
 * 这是"设备特有方法"——只属于 gpio_led，不在基类 device_ops 中。
 */
void gpio_led_set_on(struct gpio_led *led);

/**
 * gpio_led_set_off — 熄灭 LED
 */
void gpio_led_set_off(struct gpio_led *led);

/**
 * gpio_led_toggle — 翻转 LED 状态
 */
void gpio_led_toggle(struct gpio_led *led);

#endif /* __LINUX_OOP_GPIO_LED_H__ */
