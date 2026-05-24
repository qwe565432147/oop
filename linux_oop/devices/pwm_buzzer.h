/*
 * pwm_buzzer.h — PWM 有源蜂鸣器设备
 *
 * 第三个具体设备类，和 gpio_led / i2c_rgb 使用相同的基类接口。
 * 演示"同一个基类可以被无限扩展"。
 *
 * PWM 蜂鸣器和 LED 的不同：
 *   LED     只有开/关两个状态
 *   蜂鸣器  有频率（音调）和开关两个维度
 *
 * 新增了设备特有方法：
 *   pwm_buzzer_set_freq() — 设置频率（音调）
 *   pwm_buzzer_set_on()   — 开始鸣响
 *   pwm_buzzer_set_off()  — 停止鸣响
 */

#ifndef __LINUX_OOP_PWM_BUZZER_H__
#define __LINUX_OOP_PWM_BUZZER_H__

#include "device.h"

struct pwm_buzzer_device {
	struct device	base;
	int		pwm_pin;
	unsigned int	freq_hz;	/* 当前频率 (Hz) */
	unsigned int	duty_ns;	/* 高电平时间 (ns) */
	bool		state;
};

/* 向下转型 */
static inline struct pwm_buzzer_device *to_pwm_buzzer(struct device *dev)
{
	return container_of(dev, struct pwm_buzzer_device, base);
}

/* ================================================================
 *  公共接口（基类多态 + 设备特有方法）
 *  ================================================================
 */

struct device *pwm_buzzer_create(const char *name, int pwm_pin);

void pwm_buzzer_set_on(struct pwm_buzzer_device *buz);
void pwm_buzzer_set_off(struct pwm_buzzer_device *buz);
void pwm_buzzer_set_freq(struct pwm_buzzer_device *buz,
			  unsigned int freq_hz);
void pwm_buzzer_beep(struct pwm_buzzer_device *buz,
		     unsigned int freq_hz, unsigned int ms);

#endif /* __LINUX_OOP_PWM_BUZZER_H__ */
