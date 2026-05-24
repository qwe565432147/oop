/*
 * i2c_rgb.h — I2C 接口 RGB LED 设备
 *
 * 和 gpio_led 一样继承自 struct device，但操作方式完全不同：
 *   gpio_led → 直接写 GPIO 寄存器
 *   i2c_rgb  → 通过 I2C 总线发送颜色数据
 *
 * 这就是"多态"的典型应用——同样的 device_init() 调用，
 * gpio_led 写 GPIO，i2c_rgb 发 I2C。
 */

#ifndef __LINUX_OOP_I2C_RGB_H__
#define __LINUX_OOP_I2C_RGB_H__

#include "device.h"

/*
 * i2c_rgb_device — I2C RGB LED 设备
 *
 * 成员：
 *   base  — 继承自 device
 *   addr  — I2C 设备地址（7位地址）
 *   r, g, b — 当前颜色值
 *   state — 开关状态
 */
struct i2c_rgb_device {
	struct device	base;
	uint8_t		addr;	/* I2C 从机地址 */
	uint8_t		r;
	uint8_t		g;
	uint8_t		b;
	bool		state;
};

/* 向下转型：device* → i2c_rgb_device* */
static inline struct i2c_rgb_device *to_i2c_rgb(struct device *dev)
{
	return container_of(dev, struct i2c_rgb_device, base);
}

/* ================================================================
 *  公共接口
 *  ================================================================
 */

struct device *i2c_rgb_create(const char *name, uint8_t addr);

void i2c_rgb_set_on(struct i2c_rgb_device *rgb);
void i2c_rgb_set_off(struct i2c_rgb_device *rgb);
void i2c_rgb_set_color(struct i2c_rgb_device *rgb,
			uint8_t r, uint8_t g, uint8_t b);

#endif /* __LINUX_OOP_I2C_RGB_H__ */
