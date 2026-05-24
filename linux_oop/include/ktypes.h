/*
 * ktypes.h — 内核风格基础类型与宏
 *
 * 提供 container_of、ARRAY_SIZE 等 Linux 内核中最常用的基础设施宏。
 * 理解这些宏是阅读内核源码的第一步。
 *
 *   container_of(ptr, type, member)
 *       ─── 通过结构体某成员的指针，反推出整个结构体的指针
 *       这是 Linux 内核"面向对象"的基石，替代了 C++ 的 dynamic_cast
 */

#ifndef __LINUX_OOP_KTYPES_H__
#define __LINUX_OOP_KTYPES_H__

#include <stddef.h>	/* offsetof */
#include <stdint.h>
#include <stdbool.h>

/*
 * container_of — 通过成员指针获取容器结构体指针
 * @ptr:    结构体中某个成员的指针
 * @type:   容器结构体的类型
 * @member: 成员在结构体中的名字
 *
 * 原理：将成员指针向后偏移该成员在结构体中的偏移量。
 * 即：容器地址 = 成员地址 - 成员在容器中的偏移
 *
 * 这是 C 语言实现"继承-多态"体系中最核心的宏。
 * 内核中几乎每个驱动文件都会用到它。
 *
 * 示例：
 *   struct gpio_led led;
 *   struct device *dev = &led.base;   // 向上转型（安全）
 *   struct gpio_led *p = container_of(dev, struct gpio_led, base); // 向下转型
 *   // 现在 p == &led
 */
#define container_of(ptr, type, member)				\
	((type *)((char *)(ptr) - offsetof(type, member)))

/*
 * ARRAY_SIZE — 获取静态数组的元素个数
 * 在 C 中，sizeof 数组 = 元素个数 × 每个元素大小
 * 所以 sizeof(arr) / sizeof(arr[0]) = 元素个数
 *
 * 注意：只能用于真正的数组，不能用于指针！
 *   int arr[10];    // ARRAY_SIZE(arr) → 10  ✓
 *   int *p = arr;   // ARRAY_SIZE(p)   → 错误的结果 ✗
 */
#define ARRAY_SIZE(arr)	(sizeof(arr) / sizeof((arr)[0]))

/*
 * 内核风格错误码
 * 正数或 0 表示成功，负数表示错误
 * 这种风格贯穿整个 Linux 内核
 */
#define E_OK		0	/* 成功 */
#define E_PARAM		(-1)	/* 参数错误 */
#define E_NOMEM		(-2)	/* 内存不足 */
#define E_NODEV		(-3)	/* 设备不存在 */
#define E_BUSY		(-4)	/* 资源忙 */
#define E_AGAIN		(-5)	/* 请重试 */

/*
 * 内核工具宏
 */
#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))
#define MIN(a, b)		((a) < (b) ? (a) : (b))
#define MAX(a, b)		((a) > (b) ? (a) : (b))

#endif /* __LINUX_OOP_KTYPES_H__ */
