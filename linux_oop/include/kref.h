/*
 * kref.h — 内核风格引用计数
 *
 * 为什么需要引用计数？
 *   当多个模块共享同一个对象时，谁该负责释放它？
 *   引用计数的答案是：最后一个使用者负责释放。
 *
 * 工作原理：
 *   kref_init:  初始计数 = 1（表示有一个引用）
 *   kref_get:   计数 +1（表示多了一个使用者）
 *   kref_put:   计数 -1，减到 0 时调用 release 回调
 *
 * 类比：图书馆的书
 *   kref_init  = 新书入库（1人预约）
 *   kref_get   = 又有人预约（+1）
 *   kref_put   = 有人还书（-1），最后一个人还书时书被销毁
 *
 * 注意：本实现是单线程版本。多线程环境需使用原子操作
 * （见 Linux 内核的 atomic_t / refcount_t）
 */

#ifndef __LINUX_OOP_KREF_H__
#define __LINUX_OOP_KREF_H__

struct kref {
	unsigned int count;	/* 引用计数器 */
};

/**
 * kref_init — 初始化引用计数为 1
 * @kref: 目标对象
 *
 * 初始化后 count = 1，表示有一个引用。
 */
static inline void kref_init(struct kref *kref)
{
	kref->count = 1;
}

/**
 * kref_get — 增加引用计数
 * @kref: 目标对象
 *
 * 表示多了一个使用者。
 * 调用者必须确保 kref 指向的对象仍然存活。
 */
static inline void kref_get(struct kref *kref)
{
	kref->count++;
}

/**
 * kref_put — 减少引用计数
 * @kref:    目标对象
 * @release: 当计数归零时调用的释放函数
 *
 * 返回 1 表示对象已被释放，返回 0 表示还有其他人引用。
 * 调用者不能在 release 之后继续访问 kref 指向的对象。
 */
static inline int kref_put(struct kref *kref, void (*release)(struct kref *kref))
{
	if (--kref->count == 0) {
		if (release)
			release(kref);
		return 1;
	}
	return 0;
}

/**
 * kref_read — 读取当前引用计数（调试用）
 */
static inline unsigned int kref_read(const struct kref *kref)
{
	return kref->count;
}

#endif /* __LINUX_OOP_KREF_H__ */
