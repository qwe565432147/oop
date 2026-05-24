/*
 * main.c — 完整 OOP 功能演示
 *
 * 本文件展示 linux_oop 框架的所有核心特性：
 *
 *   特性                         代码示例
 *   ───────────────────────────  ──────────────────────────────
 *   1. 多态（虚表 vtable）       device_init(dev) 调用不同实现
 *   2. 继承（嵌入+container_of）  to_gpio_led(dev) 向下转型
 *   3. 侵入式链表                 device_for_each() 遍历
 *   4. 引用计数                  device_get() / device_put()
 *   5. 生命周期管理              device_register / unregister
 *   6. 批量操作                  device_suspend_all()
 *   7. 回调模式                  device_for_each(cb, data)
 *   8. 封装（不透明访问）         .h/.c 分离
 *   9. 开闭原则                  新增设备不改现有代码
 *  10. 错误处理                  goto cleanup 模式
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ktypes.h>
#include <gpio_led.h>
#include <i2c_rgb.h>
#include <pwm_buzzer.h>

/* ================================================================
 *  第一部分：回调函数（用于 device_for_each 演示）
 *  ================================================================
 */

/*
 * 打印设备信息的回调函数
 * device_for_each 会为每个设备调用这个函数
 */
static void print_device_info(struct device *dev, void *data)
{
	unsigned int *count = (unsigned int *)data;

	printf("    [%02u] %s\n", (*count)++, dev->name);
}

/*
 * 演示引用计数的回调：对每个设备调用 device_get
 */
static void refcount_get_all(struct device *dev, void *data)
{
	(void)data;
	device_get(dev);
	printf("[demo] get ref: %s (count=%u)\n",
	       dev->name, kref_read(&dev->ref));
}

/* ================================================================
 *  第二部分：主演示逻辑
 *  ================================================================
 */

int main(void)
{
	struct device	*led_dev;
	struct device	*rgb_dev;
	struct device	*buz_dev;

	struct gpio_led		*led;
	struct i2c_rgb_device	*rgb;
	struct pwm_buzzer_device *buz;

	unsigned int dev_count;

	printf("\n");
	printf("============================================\n");
	printf("  Linux 风格 C 语言 OOP 框架演示\n");
	printf("============================================\n");
	printf("\n");

	/*
	 * ════════════════════════════════════════════════
	 *  演示 1：创建多态设备
	 *
	 *  每种设备的创建函数返回 struct device *（基类指针）。
	 *  调用者不需要知道具体类型，就可以操作设备。
	 * ════════════════════════════════════════════════
	 */
	printf("--- [1] 创建设备（构造函数自动调用 init + register）---\n");
	printf("\n");

	led_dev = gpio_led_create("status_led", 13, true);
	rgb_dev = i2c_rgb_create("rgb_strip",  0x22);
	buz_dev = pwm_buzzer_create("alert_buzzer", 5);

	if (!led_dev || !rgb_dev || !buz_dev) {
		fprintf(stderr, "[demo] ERROR: device creation failed!\n");
		return -1;
	}

	printf("\n");

	/*
	 * ════════════════════════════════════════════════
	 *  演示 2：设备特有方法（需要知道具体类型）
	 *
	 *  通过 container_of（to_gpio_led / to_i2c_rgb 宏）
	 *  从基类指针转换到具体类型指针。
	 *
	 *  这是 C 语言中"向下转型"的标准做法。
	 * ════════════════════════════════════════════════
	 */
	printf("--- [2] 设备特有方法（向下转型 + 类型安全） ---\n");
	printf("\n");

	led = to_gpio_led(led_dev);
	gpio_led_set_on(led);		/* GPIO_13 -> ON */
	gpio_led_toggle(led);		/* GPIO_13 -> OFF */
	gpio_led_toggle(led);		/* GPIO_13 -> ON */

	printf("\n");

	rgb = to_i2c_rgb(rgb_dev);
	i2c_rgb_set_color(rgb, 255, 0, 0);	/* 红色 */
	i2c_rgb_set_on(rgb);			/* I2C 发送红色数据 */

	printf("\n");

	buz = to_pwm_buzzer(buz_dev);
	pwm_buzzer_beep(buz, 1000, 200);	/* 1000Hz 叫 200ms */
	pwm_buzzer_beep(buz, 2000, 100);	/* 2000Hz 叫 100ms */

	printf("\n");

	/*
	 * ════════════════════════════════════════════════
	 *  演示 3：多态操作（device_find + 统一接口）
	 *
	 *  通过 device_find 查找设备，然后通过基类指针调用多态方法。
	 *  调用者完全不关心底层是 LED 还是 RGB。
	 * ════════════════════════════════════════════════
	 */
	printf("--- [3] 多态操作（统一接口，不同行为） ---\n");
	printf("\n");

	{
		struct device *dev;
		const char *names[] = { "status_led",
					"rgb_strip",
					"alert_buzzer" };

		for (size_t i = 0; i < ARRAY_SIZE(names); i++) {
			dev = device_find(names[i]);
			if (dev) {
				printf("[demo] found: %s\n", device_name(dev));

				/*
				 * 对同一个 device_init() 调用，
				 * 三个设备各自执行不同的代码：
				 *
				 *   status_led  → printf("init GPIO_13...")
				 *   rgb_strip   → printf("init I2C 0x22...")
				 *   alert_buzzer→ printf("init PWM 5...")
				 *
				 * init 只会被调用一次，因为 device_register
				 * 时已经调用过了，这里只是演示多态接口可用。
				 */
				device_suspend(dev);
				device_resume(dev);
			}
		}
	}

	printf("\n");

	/*
	 * ════════════════════════════════════════════════
	 *  演示 4：批量管理（遍历设备链表）
	 *
	 *  device_for_each + 回调函数，让外部代码扩展内部遍历逻辑。
	 *  这是"策略模式"的一种应用。
	 * ════════════════════════════════════════════════
	 */
	printf("--- [4] 批量管理（遍历 + 回调） ---\n");
	printf("\n");

	dev_count = 0;
	device_for_each(print_device_info, &dev_count);

	printf("\n");

	/*
	 * ════════════════════════════════════════════════
	 *  演示 5：引用计数
	 *
	 *  引用计数管理共享对象的生命周期。
	 *  这里演示 device_get / device_put / kref_read。
	 * ════════════════════════════════════════════════
	 */
	printf("--- [5] 引用计数（共享对象生命周期管理） ---\n");
	printf("\n");

	printf("[demo] initial refcount: %u\n",
	       kref_read(&led_dev->ref));

	/* 通过 device_for_each 对每个设备调用 device_get */
	device_for_each(refcount_get_all, NULL);

	printf("[demo] after device_for_each get: count=%u\n",
	       kref_read(&led_dev->ref));

	device_put(led_dev);	/* -1 */
	printf("[demo] after 1x put:  refcount=%u\n",
	       kref_read(&led_dev->ref));

	printf("\n");

	/*
	 * ════════════════════════════════════════════════
	 *  演示 6：批量挂起/恢复（多态的典型应用）
	 *
	 *  一次调用，影响所有设备。每个设备执行自己的 suspend/resume。
	 *  这就是 OOP 多态的威力——新増设备自动支持这些操作。
	 * ════════════════════════════════════════════════
	 */
	printf("--- [6] 批量挂起/恢复 ---\n");
	printf("\n");

	printf("[demo] === SUSPEND ALL ===\n");
	device_suspend_all();
	printf("\n");

	printf("[demo] === RESUME ALL ===\n");
	device_resume_all();

	printf("\n");

	/*
	 * ════════════════════════════════════════════════
	 *  演示 7：设备注销与生命周期结束
	 *
	 *  device_unregister → 从链表移除 → 引用减1
	 *  → 引用归零 → 自动调用 ops->destroy
	 * ════════════════════════════════════════════════
	 */
	printf("--- [7] 设备注销（引用归零 → 自动销毁） ---\n");
	printf("\n");

	printf("[demo] unregistering all devices...\n");

	/*
	 * 注意：device_unregister 会调用 device_put，
	 * 而我们的设备在 create 时 ref=1，所以 unregister 就触发 destroy。
	 *
	 * 但如果之前有 device_get 没有对应的 device_put，
	 * 就不会释放。这就是引用计数的意义——确保最后一个使用者释放。
	 */
	device_unregister(led_dev);
	device_unregister(rgb_dev);
	device_unregister(buz_dev);

	/*
	 * 注意：unregister 之后，设备列表应该为空
	 */
	printf("\n");
	printf("[demo] checking device list after unregister:\n");
	dev_count = 0;
	device_for_each(print_device_info, &dev_count);
	if (dev_count == 0)
		printf("    (empty — all devices cleaned up)\n");

	printf("\n");
	printf("============================================\n");
	printf("  演示完成\n");
	printf("============================================\n");
	printf("\n");

	return 0;
}
