/*
 * i2c_rgb.c — I2C RGB LED 设备实现
 *
 * 和 gpio_led 同样的 device_ops 接口，不同的底层实现。
 * 这正是多态的威力：上层代码无需知道具体实现细节。
 */

#include "i2c_rgb.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * "发送颜色数据到 I2C 设备"的模拟
 * 在真实驱动中，这里会调用 i2c_master_write() 之类的函数
 */
static void i2c_send_color(struct i2c_rgb_device *rgb)
{
	printf("[i2c_rgb] %s: I2C:0x%02x << R=%u G=%u B=%u%s\n",
	       rgb->base.name, rgb->addr,
	       rgb->r, rgb->g, rgb->b,
	       rgb->state ? " ON" : " OFF");
}

/*
 * device_ops 虚函数实现
 */
static int rgb_dev_init(struct device *dev)
{
	struct i2c_rgb_device *rgb = to_i2c_rgb(dev);

	/* 默认颜色：白色（全亮） */
	rgb->r = rgb->g = rgb->b = 255;
	rgb->state = false;

	printf("[i2c_rgb] %s: init I2C addr=0x%02x\n",
	       dev->name, rgb->addr);
	return E_OK;
}

static void rgb_dev_destroy(struct device *dev)
{
	struct i2c_rgb_device *rgb = to_i2c_rgb(dev);

	i2c_rgb_set_off(rgb);
	printf("[i2c_rgb] %s: destroy I2C addr=0x%02x\n",
	       dev->name, rgb->addr);
	free(rgb);
}

/*
 * 虚函数表：和 gpio_led 完全相同的 struct device_ops 类型，
 * 但是绑定了不同的实现函数
 */
static const struct device_ops i2c_rgb_ops = {
	.init    = rgb_dev_init,
	.destroy = rgb_dev_destroy,
};

/*
 * 设备特有方法实现
 */
void i2c_rgb_set_on(struct i2c_rgb_device *rgb)
{
	if (!rgb)
		return;

	rgb->state = true;
	i2c_send_color(rgb);
}

void i2c_rgb_set_off(struct i2c_rgb_device *rgb)
{
	if (!rgb)
		return;

	rgb->state = false;
	i2c_send_color(rgb);
}

void i2c_rgb_set_color(struct i2c_rgb_device *rgb,
			uint8_t r, uint8_t g, uint8_t b)
{
	if (!rgb)
		return;

	rgb->r = r;
	rgb->g = g;
	rgb->b = b;
	printf("[i2c_rgb] %s: set color (%u,%u,%u)\n",
	       rgb->base.name, r, g, b);
}

/*
 * 构造函数
 */
struct device *i2c_rgb_create(const char *name, uint8_t addr)
{
	struct i2c_rgb_device *rgb;
	int ret;

	rgb = malloc(sizeof(*rgb));
	if (!rgb) {
		printf("[i2c_rgb] failed to allocate %s\n", name);
		return NULL;
	}

	rgb->base.ops	= &i2c_rgb_ops;
	rgb->base.name	= name;
	rgb->addr	= addr;
	kref_init(&rgb->base.ref);

	ret = device_register(&rgb->base);
	if (ret) {
		printf("[i2c_rgb] failed to register %s: err=%d\n",
		       name, ret);
		free(rgb);
		return NULL;
	}

	return &rgb->base;
}
