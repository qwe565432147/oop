/*
 * device.h — 抽象设备基类（Linux 内核风格设备模型）
 *
 * 本文件是整个 OOP 框架的核心，定义了：
 *   1. device_ops — 虚函数表（类比 Linux 的 file_operations）
 *   2. device     — 抽象基类（类比 Linux 的 struct device）
 *   3. 多态操作接口（device_init / device_suspend / device_resume 等）
 *   4. 设备注册与管理（设备链表 + 按名查找）
 *
 * 设计思路：
 *   ┌─────────────────────────────────────────────────┐
 *   │                struct device                      │
 *   │  ┌─────────────┬──────────┬──────────────┬─────┐ │
 *   │  │ node (链表) │ ref (计数)│ name (名称)  │ ops │ │
 *   │  └─────────────┴──────────┴──────────────┴─────┘ │
 *   │                     ▲                             │
 *   │                     │ 继承（嵌入）                  │
 *   │       ┌─────────────┼─────────────┐               │
 *   │       │             │             │               │
 *   │  gpio_led    i2c_rgb     pwm_buzzer               │
 *   └─────────────────────────────────────────────────┘
 *
 * 对比 Linux 内核：
 *   device.h 里的 device      → 内核的 struct device
 *   device_ops                → 内核的 struct file_operations
 *   device_register           → 内核的 device_register()
 *   device_find               → 内核的 bus_find_device()
 *   container_of 向下转型     → 内核的 to_*_device() 宏
 */

#ifndef __LINUX_OOP_DEVICE_H__
#define __LINUX_OOP_DEVICE_H__

#include <ktypes.h>
#include <klist.h>
#include <kref.h>

/*
 * 前向声明：struct device 在 device_ops 之后才定义，
 * 但 device_ops 的函数指针参数需要用到 struct device *。
 * 前向声明告诉编译器"struct device 存在，详细定义在后面"。
 *
 * 这是 C 语言中处理"互相引用"的标准手法。
 * 内核源码中随处可见。
 */
struct device;

/*
 * 虚函数表 / 操作表（类比 Linux 的 struct file_operations）
 *
 * 这是实现"多态"的关键。每个具体设备类型提供自己的实现：
 *
 *   static const struct device_ops gpio_led_ops = {
 *           .init    = gpio_led_init,
 *           .suspend = gpio_led_suspend,
 *           .resume  = gpio_led_resume,
 *           .destroy = gpio_led_destroy,
 *   };
 *
 * 所谓"虚函数"就是通过指针间接调用函数：
 *   dev->ops->init(dev);
 *   运行时才会确定调用的是 gpio_led_init 还是 i2c_rgb_init
 */
struct device_ops {
	/*
	 * init — 初始化设备（在注册时自动调用）
	 * 返回 0 成功，负数失败
	 */
	int (*init)(struct device *dev);

	/*
	 * suspend — 挂起设备（进入低功耗）
	 * 在系统休眠时由电源管理模块调用
	 */
	void (*suspend)(struct device *dev);

	/*
	 * resume — 恢复设备（从低功耗唤醒）
	 * suspend 的逆操作
	 */
	void (*resume)(struct device *dev);

	/*
	 * destroy — 销毁设备（释放资源）
	 * 在引用计数归零时由 kref_put 回调调用
	 */
	void (*destroy)(struct device *dev);
};

/*
 * device — 抽象基类
 *
 * 所有"设备"的共同祖先。包含：
 *   node   — 链表节点，用于将设备注册到全局设备链表
 *   ref    — 引用计数，管理设备生命周期
 *   name   — 设备名称，用于查找和调试
 *   ops    — 虚函数表指针，实现多态
 *
 * "抽象基类"的意思是：你不能直接创建 device，必须用具体设备类型。
 * 就像你不能直接创建"动物"，但可以创建"猫"或"狗"。
 */
struct device {
	struct list_head	node;	/* 设备链表节点，嵌入式的侵入式节点 */
	struct kref		ref;	/* 引用计数 */
	const char		*name;	/* 设备名称 */
	const struct device_ops	*ops;	/* 虚函数表指针 → 多态的关键 */
};

/* ================================================================
 *  多态操作接口
 *
 *  这些是暴露给使用者的"统一操作接口"。
 *  不管底层是什么设备类型，调用方式完全一样：
 *
 *    struct device *dev = device_find("status_led");
 *    device_init(dev);      // 调用 gpio_led 或 i2c_rgb 的 init
 *    device_suspend(dev);   // 调用相应设备的 suspend
 *
 *  使用者不需要知道具体类型，这就是 OOP 的"多态"。
 *  ================================================================
 */

/**
 * device_init — 初始化设备（多态）
 * @dev: 设备对象
 * Return: 0 成功，负数失败
 *
 * 通过 ops->init 间接调用具体设备的初始化函数。
 */
static inline int device_init(struct device *dev)
{
	if (!dev || !dev->ops || !dev->ops->init)
		return -E_PARAM;
	return dev->ops->init(dev);
}

/**
 * device_suspend — 挂起设备（多态）
 */
static inline void device_suspend(struct device *dev)
{
	if (dev && dev->ops && dev->ops->suspend)
		dev->ops->suspend(dev);
}

/**
 * device_resume — 恢复设备（多态）
 */
static inline void device_resume(struct device *dev)
{
	if (dev && dev->ops && dev->ops->resume)
		dev->ops->resume(dev);
}

/**
 * device_destroy — 销毁设备（多态）
 */
static inline void device_destroy(struct device *dev)
{
	if (dev && dev->ops && dev->ops->destroy)
		dev->ops->destroy(dev);
}

/* ================================================================
 *  设备生命周期管理
 *  ================================================================
 */

/**
 * device_register — 注册设备到系统
 * @dev: 设备对象
 * Return: 0 成功，负数失败
 *
 * 注册过程：
 *   1. 调用 dev->ops->init() 初始化硬件
 *   2. 将设备加入全局设备链表
 *
 * 初始化 + 注册分离的好处：
 *   创建对象、初始化硬件、注册到系统 是三个独立步骤，
 *   可以在不同时间、由不同模块完成。
 */
int device_register(struct device *dev);

/**
 * device_unregister — 从系统注销设备
 * @dev: 设备对象
 *
 * 注销过程：
 *   1. 从全局设备链表移除
 *   2. 调用 dev->ops->destroy() 释放硬件资源
 *   3. 减少引用计数
 */
void device_unregister(struct device *dev);

/* ================================================================
 *  引用计数管理
 *  ================================================================
 */

/**
 * device_get — 增加设备引用计数
 * @dev: 设备对象
 * Return: 返回 dev 本身（方便链式调用）
 */
static inline struct device *device_get(struct device *dev)
{
	if (dev)
		kref_get(&dev->ref);
	return dev;
}

/**
 * device_put — 减少设备引用计数
 * @dev: 设备对象
 *
 * 当引用计数归零时，自动调用 dev->ops->destroy。
 */
void device_put(struct device *dev);

/* ================================================================
 *  设备查找与管理
 *  ================================================================
 */

/**
 * device_find — 按名称查找设备
 * @name: 设备名称
 * Return: 设备指针，未找到返回 NULL
 *
 * 遍历全局设备链表，逐一比较设备名。
 * 时间复杂度 O(n)，适用于设备数量不多的嵌入式场景。
 * 内核中使用更高效的哈希表和 bus/class 结构。
 */
struct device *device_find(const char *name);

/**
 * device_suspend_all — 挂起所有已注册设备
 *
 * 遍历设备链表，逐一调用设备的 suspend 函数。
 * 演示"统一接口操作所有对象"的多态应用。
 */
void device_suspend_all(void);

/**
 * device_resume_all — 恢复所有已注册设备
 */
void device_resume_all(void);

/**
 * device_for_each — 遍历所有设备并调用回调函数
 * @cb:   回调函数
 * @data: 透传给回调的私有数据
 *
 * 演示"回调 + 遍历"的解耦模式。
 * 回调函数由调用者提供，device_for_each 不关心回调怎么实现。
 */
void device_for_each(void (*cb)(struct device *dev, void *data), void *data);

/**
 * device_name — 获取设备名称（带空保护）
 */
static inline const char *device_name(struct device *dev)
{
	return dev ? dev->name : "(null)";
}

#endif /* __LINUX_OOP_DEVICE_H__ */
