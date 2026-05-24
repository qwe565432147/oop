/*
 * klist.h — 内核风格侵入式双向链表
 *
 * 这是 Linux 内核中最核心、使用最广泛的数据结构。
 * 学习本文件前，请先理解两个关键区别：
 *
 *   普通链表（教学版）          侵入式链表（内核版）
 *   ─────────────────          ─────────────────
 *   链表节点中存数据            数据中嵌入链表节点
 *   struct list {               struct my_obj {
 *       int data;                   struct list_head node;
 *       struct list *next;          int data;
 *       struct list *prev;      };
 *   };
 *
 *   一个数据只能在一个链表中    一个数据可以在多个链表中
 *   遍历时要复制数据            遍历时直接得到原始对象
 *
 * 侵入式链表的"侵入"是指：你的数据结构必须"侵入"一个 list_head 成员。
 * 看似增加了耦合，实际上获得了极大的灵活性。
 */

#ifndef __LINUX_OOP_KLIST_H__
#define __LINUX_OOP_KLIST_H__

#include "ktypes.h"

/*
 * list_head — 链表节点（同时也是链表头）
 *
 * 这是一个双向链表节点。它不包含"数据"，
 * 数据通过 container_of 从节点反推出来。
 *
 * 空链表：next 和 prev 都指向自己
 *
 *    head
 *    ┌──┬──┐
 *    │next───┐
 *    │prev───┤
 *    └──┴──┘  │
 *       ↑_____│  (自环)
 *
 * 非空链表：
 *    head <-> node1 <-> node2 <-> head (循环)
 */
struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

/*
 * LIST_HEAD_INIT — 编译期静态初始化链表头
 * LIST_HEAD    — 声明并初始化链表头
 *
 * 用法：
 *   // 方法1：静态声明
 *   LIST_HEAD(my_list);
 *
 *   // 方法2：动态初始化
 *   struct list_head my_list;
 *   INIT_LIST_HEAD(&my_list);
 */
#define LIST_HEAD_INIT(name)	{ &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

/*
 * INIT_LIST_HEAD — 运行时初始化链表头
 * 作用和 LIST_HEAD 一样，但用于已声明的变量
 */
static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * 内部函数：在两个已知节点之间插入
 * 不直接调用，由 list_add / list_add_tail 使用
 */
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next  = next;
	new->prev  = prev;
	prev->next = new;
}

/*
 * list_add — 在链表头部插入（头插法）
 * 相当于栈的 push 操作
 *
 *   head <-> new <-> old_first <-> ...
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

/*
 * list_add_tail — 在链表尾部插入（尾插法）
 * 相当于队列的 enqueue 操作
 *
 *   head <-> ... <-> old_last <-> new <-> head
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * 内部函数：删除两个已知节点之间的节点
 */
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/*
 * list_del — 从链表中删除一个节点
 * 删除后节点的 prev/next 变为 LIST_POISON（未定义），
 * 不再指向原来的链表
 */
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

/*
 * list_del_init — 删除节点并重新初始化
 * 和 list_del 的区别：删除后节点可以重新加入链表
 */
static inline void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

/*
 * list_replace — 替换一个节点
 */
static inline void list_replace(struct list_head *old, struct list_head *new)
{
	new->next = old->next;
	new->prev = old->prev;
	new->next->prev = new;
	new->prev->next = new;
}

/*
 * list_empty — 判断链表是否为空
 * 空链表条件：head->next == head
 */
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

/*
 * list_is_singular — 判断链表是否只有一个元素
 */
static inline int list_is_singular(const struct list_head *head)
{
	return !list_empty(head) && (head->next == head->prev);
}

/*
 * list_splice — 将 list 链表拼接到 head 后面
 */
static inline void list_splice(struct list_head *list,
			       struct list_head *head)
{
	struct list_head *first = list->next;
	struct list_head *last  = list->prev;

	if (list_empty(list))
		return;

	first->prev = head;
	last->next  = head->next;
	head->next->prev = last;
	head->next = first;
}

/*
 * list_entry — 通过链表节点指针获取容器结构体指针
 *
 *   ptr:   struct list_head *（链表节点指针）
 *   type:  容器类型（例如 struct gpio_led）
 *   member: 链表节点在容器中的成员名（例如 node）
 *
 * 本质上就是 container_of 的别名
 *
 * 用法：
 *   struct gpio_led *led = list_entry(node_ptr, struct gpio_led, node);
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/*
 * list_first_entry — 获取链表中第一个元素
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

/*
 * list_last_entry — 获取链表中最后一个元素
 */
#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

/*
 * list_next_entry — 获取当前元素的下一个元素
 */
#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

/*
 * list_prev_entry — 获取当前元素的上一个元素
 */
#define list_prev_entry(pos, member) \
	list_entry((pos)->member.prev, typeof(*(pos)), member)

/*
 * list_for_each — 遍历链表节点（低级，不常用）
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/*
 * list_for_each_entry — 遍历链表元素（最常用的遍历宏）
 *
 *   pos:    循环变量（容器类型指针）
 *   head:   链表头
 *   member: list_head 在容器中的成员名
 *
 * 用法：
 *   struct device *dev;
 *   list_for_each_entry(dev, &device_list, node) {
 *           printf("%s\n", dev->name);
 *   }
 *
 * 展开逻辑：
 *   1. pos = 第一个元素的地址
 *   2. 检查 pos->member 是否回到了 head
 *   3. pos = 下一个元素的地址
 */
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

/*
 * list_for_each_entry_safe — 安全遍历（允许删除当前元素）
 *
 * 和 list_for_each_entry 的唯一区别是多了一个临时变量 n，
 * 用于在删除 pos 后仍能找到下一个元素。
 *
 *   为什么需要 safe 版本？
 *   如果在遍历中删除 pos，pos->member.next 可能无效。
 *   所以先用 n 保存下一个元素的位置。
 *
 * 用法：
 *   struct device *dev, *tmp;
 *   list_for_each_entry_safe(dev, tmp, &device_list, node) {
 *           if (some_condition(dev))
 *                   list_del(&dev->node);
 *   }
 */
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
	     n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = n,							\
	     n = list_entry(n->member.next, typeof(*n), member))

#endif /* __LINUX_OOP_KLIST_H__ */
