/*
 * pwm_buzzer.c — PWM 蜂鸣器设备实现
 *
 * 第三个具体设备类，进一步展示"对扩展开放，对修改关闭"的开闭原则。
 *
 * 扩展 pwm_buzzer 不需要修改：
 *   - device.h（基类定义）
 *   - device.c（设备管理核心）
 *   - 其他设备（gpio_led, i2c_rgb）
 *
 * 只需要：
 *   - 新建 pwm_buzzer.h/c
 *   - 提供 device_ops 实现
 *   - 提供设备特有方法
 */

#include "pwm_buzzer.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * 模拟 PWM 输出
 * 在真实驱动中会配置硬件 PWM 定时器
 */
static void pwm_set_freq_hw(struct pwm_buzzer_device *buz)
{
	printf("[pwm_buzzer] %s: PWM:%d set freq=%u Hz, duty=%u ns%s\n",
	       buz->base.name, buz->pwm_pin, buz->freq_hz, buz->duty_ns,
	       buz->state ? " (BUZZ)" : " (OFF)");
}

/*
 * device_ops 实现
 */
static int buzzer_dev_init(struct device *dev)
{
	struct pwm_buzzer_device *buz = to_pwm_buzzer(dev);

	buz->freq_hz  = 0;
	buz->duty_ns  = 0;
	buz->state    = false;

	printf("[pwm_buzzer] %s: init PWM:%d\n", dev->name, buz->pwm_pin);
	return E_OK;
}

static void buzzer_dev_destroy(struct device *dev)
{
	struct pwm_buzzer_device *buz = to_pwm_buzzer(dev);

	pwm_buzzer_set_off(buz);
	printf("[pwm_buzzer] %s: destroy PWM:%d\n", dev->name, buz->pwm_pin);
	free(buz);
}

static const struct device_ops pwm_buzzer_ops = {
	.init    = buzzer_dev_init,
	.destroy = buzzer_dev_destroy,
};

/*
 * 设备特有方法
 */
void pwm_buzzer_set_on(struct pwm_buzzer_device *buz)
{
	if (!buz)
		return;

	buz->state = true;
	pwm_set_freq_hw(buz);
}

void pwm_buzzer_set_off(struct pwm_buzzer_device *buz)
{
	if (!buz)
		return;

	buz->state = false;
	pwm_set_freq_hw(buz);
}

void pwm_buzzer_set_freq(struct pwm_buzzer_device *buz,
			 unsigned int freq_hz)
{
	if (!buz || freq_hz == 0)
		return;

	buz->freq_hz = freq_hz;
	buz->duty_ns = 500000000U / freq_hz; /* 50% 占空比，简化计算 */

	printf("[pwm_buzzer] %s: set freq=%u Hz\n", buz->base.name, freq_hz);

	/* 如果正在鸣响，立即更新频率 */
	if (buz->state)
		pwm_set_freq_hw(buz);
}

void pwm_buzzer_beep(struct pwm_buzzer_device *buz,
		     unsigned int freq_hz, unsigned int ms)
{
	if (!buz || freq_hz == 0)
		return;

	pwm_buzzer_set_freq(buz, freq_hz);
	pwm_buzzer_set_on(buz);

	/*
	 * 在真实驱动中，这里应该启动一个定时器
	 * 在 ms 毫秒后自动关闭
	 */
	printf("[pwm_buzzer] %s: beep at %u Hz for %u ms\n",
	       buz->base.name, freq_hz, ms);

	pwm_buzzer_set_off(buz);
}

/*
 * 构造函数
 */
struct device *pwm_buzzer_create(const char *name, int pwm_pin)
{
	struct pwm_buzzer_device *buz;
	int ret;

	buz = malloc(sizeof(*buz));
	if (!buz) {
		printf("[pwm_buzzer] failed to allocate %s\n", name);
		return NULL;
	}

	buz->base.ops	= &pwm_buzzer_ops;
	buz->base.name	= name;
	buz->pwm_pin	= pwm_pin;
	kref_init(&buz->base.ref);

	ret = device_register(&buz->base);
	if (ret) {
		printf("[pwm_buzzer] failed to register %s: err=%d\n",
		       name, ret);
		free(buz);
		return NULL;
	}

	return &buz->base;
}
