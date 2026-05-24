/*
 * 01_funcptr.c —— 第1步：函数指针 = 对象的方法
 *
 * 目标：把函数也装进结构体里，让"对象"既有数据又有行为。
 *
 * Python：
 *   class Dog:
 *       def __init__(self, name):
 *           self.name = name
 *       def bark(self):          # 方法 = 绑在对象上的函数
 *           print(f"{self.name}: 汪汪")
 *   d = Dog("旺财")
 *   d.bark()     # 对象.方法()
 *
 * C的关键技巧：函数指针作为结构体成员。
 *   就像 int age 存了一个年龄数字一样，
 *   函数指针存了一个函数的地址。
 */

#include <stdio.h>

/* 结构体定义必须在函数之前，否则函数无法访问成员 */
struct dog {
    const char *name;   /* 数据 */
    void (*bark)(struct dog *);   /* 方法：函数指针 */
};

/* 具体的函数实现 */
static void bark_cn(struct dog *d)
{
    printf("%s: 汪汪！\n", d->name);
}

static void bark_en(struct dog *d)
{
    printf("%s: Woof!\n", d->name);
}

int main(void)
{
    /* 初始化时把函数地址"绑"进去 */
    struct dog wangcai = {"旺财", bark_cn};
    struct dog buddy   = {"Buddy", bark_en};

    /*
     * 调用：对象.方法(对象自己)
     *
     * 可以类比成 Python 的 wangcai.bark()
     * Python 在背后也是 self 参数
     */
    wangcai.bark(&wangcai);   /* 输出：旺财：汪汪！ */
    buddy.bark(&buddy);       /* 输出：Buddy: Woof! */

    return 0;
}

/*
 * ─── 理解检查 ───
 * 1. struct dog 现在既有 name（数据）又有 bark（行为）
 * 2. 不同对象可以"绑"不同的函数（wangcai 叫中文，buddy 叫英文）
 * 3. 调用时必须传 &wangcai 进去，因为函数需要知道"谁在叫"
 *
 * ─── 动手改改看 ───
 * 1. 加一个 eat 函数指针
 * 2. 让旺财吃骨头，buddy 吃 steak
 */
