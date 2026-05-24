/*
 * device.c — 设备管理核心实现
 *
 * 本文件实现设备注册、注销、查找、批量管理等功能的底层逻辑。
 * 全局设备链表是所有设备的"家"，device_register 让设备入住，
 * device_unregister 让设备退房。
 */

#include "device.h"
#include <stdio.h>
#include <string.h>

/*
 * 全局设备链表头
 *
 * static + 文件作用域 = 真正的封装。
 * 外部文件完全看不见这个变量，只能通过暴露的函数操作设备。
 *
 * 初始化使用 LIST_HEAD() 宏，效果等同于：
 *   struct list_head device_list = { &device_list, &device_list };
 */
static LIST_HEAD(device_list);

/*
 * 设备 ID 计数器：每次注册新设备时，如果名称冲突自动生成唯一名
 * 这也是 static 文件作用域，外部不可见
 */
static unsigned int dev_id_counter;

int device_register(struct device *dev)
{
	int ret;

	/* ---- 参数校验：防御式编程 ---- */
	if (!dev)
		return -E_PARAM;

	if (!dev->ops)
		return -E_PARAM;

	/* ---- 生命周期：初始化 ---- */
	if (dev->ops->init) {
		ret = dev->ops->init(dev);
		if (ret) {
			printf("[device] init failed for '%s': err=%d\n",
			       dev->name ? dev->name : "(unnamed)", ret);
			return ret;
		}
	}

	/*
	 * 命名：如果没有名称，自动生成一个
	 * 内核中也有类似的 deferred naming 机制
	 */
	if (!dev->name) {
		static char fallback_name[32];
		dev_id_counter++;
		snprintf(fallback_name, sizeof(fallback_name),
			 "device#%u", dev_id_counter);
		dev->name = fallback_name;
	}

	/* ---- 注册：加入全局链表 ---- */
	INIT_LIST_HEAD(&dev->node);
	list_add_tail(&dev->node, &device_list);

	printf("[device] registered: %s\n", dev->name);
	return E_OK;
}

void device_unregister(struct device *dev)
{
	if (!dev)
		return;

	/* 从链表中移除 */
	list_del_init(&dev->node);

	printf("[device] unregistered: %s\n", dev->name);

	/* 释放引用（如果 count 减到 0 会调用 destroy） */
	device_put(dev);
}

/*
 * device_kref_release — kref 回调：当引用归零时调用
 *
 * 为什么需要这个中间函数？
 *   kref_put 的回调类型是 void (*)(struct kref *)，
 *   而 ops->destroy 的类型是 void (*)(struct device *)。
 *   类型不匹配，不能强行转换（而且 struct kref 不在 device 的 offset 0）。
 *
 * 解决方案：用 container_of 从 kref* 反推出 device*，
 * 再调用 ops->destroy。
 *
 * 这种"适配器"模式在内核中很常见：
 *   回调函数签名不匹配时，加一层薄封装做类型转换。
 */
static void device_kref_release(struct kref *ref)
{
	struct device *dev = container_of(ref, struct device, ref);
	device_destroy(dev);
}

void device_put(struct device *dev)
{
	if (!dev)
		return;

	kref_put(&dev->ref, device_kref_release);
}

struct device *device_find(const char *name)
{
	struct device *dev;

	if (!name)
		return NULL;

	/*
	 * 遍历全局设备链表，逐一比较名称
	 *
	 * list_for_each_entry 在这里展开为：
	 *   for (dev = 第一个设备的地址;
	 *        dev->node 的地址 != 链表头地址;
	 *        dev = 下一个设备的地址)
	 *
	 * 注意：list_for_each_entry 返回的是"容器"（struct device）的指针，
	 * 而不是 list_head 的指针。这是侵入式链表的美妙之处。
	 */
	list_for_each_entry(dev, &device_list, node) {
		if (strcmp(dev->name, name) == 0)
			return dev;
	}

	return NULL;
}

void device_suspend_all(void)
{
	struct device *dev;

	list_for_each_entry(dev, &device_list, node) {
		device_suspend(dev);	/* 多态调用 */
	}
}

void device_resume_all(void)
{
	struct device *dev;

	list_for_each_entry(dev, &device_list, node) {
		device_resume(dev);	/* 多态调用 */
	}
}

void device_for_each(void (*cb)(struct device *dev, void *data), void *data)
{
	struct device *dev;

	if (!cb)
		return;

	list_for_each_entry(dev, &device_list, node) {
		cb(dev, data);
	}
}
