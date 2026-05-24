/*
 * gpio_led.c — GPIO LED 设备实现
 *
 * 本文件展示"具体设备类"的完整实现模式：
 *   1. 设备特有函数（static，文件作用域）
 *   2. 虚函数表实例化（device_ops 的具体实现）
 *   3. 构造函数 + 初始化
 */

#include "gpio_led.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * 第一步：实现 device_ops 中声明的方法
 *
 * 注意：这些函数的签名必须和 device_ops 中的函数指针签名一致。
 *   int  (*init)(struct device *dev);
 *   void (*destroy)(struct device *dev);
 *
 * 参数是 struct device *（基类指针），
 * 函数内部通过 container_of 转到具体类型。
 */

static int gpio_led_dev_init(struct device *dev)
{
	struct gpio_led *led = to_gpio_led(dev);

	/*
	 * 在"真实"驱动中，这里应该：
	 *   1. 请求 GPIO 资源（gpio_request）
	 *   2. 设置 GPIO 方向为输出（gpio_direction_output）
	 *   3. 初始状态为关闭
	 */
	led->state = false;
	printf("[gpio_led] %s: init GPIO_%d (active_%s)\n",
	       dev->name, led->gpio_num,
	       led->active_high ? "high" : "low");
	return E_OK;
}

static void gpio_led_dev_destroy(struct device *dev)
{
	struct gpio_led *led = to_gpio_led(dev);

	/*
	 * 在"真实"驱动中，这里应该：
	 *   1. 关闭 LED
	 *   2. 释放 GPIO 资源（gpio_free）
	 */
	gpio_led_set_off(led);
	printf("[gpio_led] %s: destroy GPIO_%d\n", dev->name, led->gpio_num);

	/*
	 * 释放 gpio_led_create 中 malloc 分配的内存。
	 * 重要：free 之前不能再访问 led 的任何成员！
	 */
	free(led);
}

/*
 * 第二步：定义虚函数表实例
 *
 * const 表示虚表在只读区（Flash/ROM），所有实例共享。
 * static 表示这个符号只在当前文件可见。
 *
 * 如果某个方法不需要，可以不指定（C 的 designated initializer
 * 会自动填 NULL），调用时会做空指针检查。
 */
static const struct device_ops gpio_led_ops = {
	.init    = gpio_led_dev_init,
	.destroy = gpio_led_dev_destroy,
	/* suspend/resume 未实现 = NULL，调用时会被安全跳过 */
};

/*
 * 第三步：设备特有方法的具体实现
 *
 * 这些是 LED 特有的操作，不在基类 device_ops 中。
 * 外部调用者必须知道具体类型才能调用。
 */

void gpio_led_set_on(struct gpio_led *led)
{
	if (!led)
		return;

	led->state = true;
	printf("[gpio_led] %s: GPIO_%d -> ON (%s)\n",
	       led->base.name, led->gpio_num,
	       led->active_high ? "HIGH" : "LOW");
}

void gpio_led_set_off(struct gpio_led *led)
{
	if (!led)
		return;

	led->state = false;
	printf("[gpio_led] %s: GPIO_%d -> OFF (%s)\n",
	       led->base.name, led->gpio_num,
	       led->active_high ? "LOW" : "HIGH");
}

void gpio_led_toggle(struct gpio_led *led)
{
	if (!led)
		return;

	if (led->state)
		gpio_led_set_off(led);
	else
		gpio_led_set_on(led);
}

/*
 * 第四步："构造函数"
 *
 * 这个函数完成三件事：
 *   1. 分配内存（malloc）—— 但如果想避免动态分配，可以接受外部传入的指针
 *   2. 初始化字段
 *   3. 注册到系统（调用 device_register）
 *
 * 内核驱动中，probe() 函数也做类似的事情：
 *   1. 分配驱动私有数据结构体
 *   2. 初始化硬件
 *   3. 注册到内核设备模型
 */
struct device *gpio_led_create(const char *name,
			       int gpio_num, bool active_high)
{
	struct gpio_led *led;
	int ret;

	/* ---- 分配 ---- */
	led = malloc(sizeof(*led));
	if (!led) {
		printf("[gpio_led] failed to allocate %s\n", name);
		return NULL;
	}

	/* ---- 初始化字段 ---- */
	led->base.ops		= &gpio_led_ops;
	led->base.name		= name;
	led->gpio_num		= gpio_num;
	led->active_high	= active_high;
	led->state		= false;
	kref_init(&led->base.ref);	/* 引用计数起始 = 1 */

	/* ---- 注册到系统 ---- */
	ret = device_register(&led->base);
	if (ret) {
		printf("[gpio_led] failed to register %s: err=%d\n",
		       name, ret);
		free(led);
		return NULL;
	}

	return &led->base;
}

/*
 * 补充说明：为什么不把构造函数命名为 gpio_led_init？
 *
 * 因为 device_ops 里已经有一个 init 了（虚函数表中的初始化方法），
 * 如果命名冲突会造成混淆。
 *
 * 内核惯例：
 *   gpio_led_create()  — 分配 + 初始化 + 注册（一次完成）
 *   gpio_led_probe()   — 内核驱动探测函数（类似 create）
 *   gpio_led_remove()  — 内核驱动移除函数
 *   dev->ops->init     — 虚函数表中的初始化（由注册过程自动调用）
 */
