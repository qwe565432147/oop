/*
 * 03_inherit.c —— 第3步：继承（Inheritance）
 *
 * 目标：理解"继承"在 C 中就是"把基类嵌入派生类"。
 *
 * 现实类比：
 *   动物（Animal）：有名字、有年龄、会叫
 *     ├── 狗（Dog）：是动物 + 会看门
 *     └── 猫（Cat）：是动物 + 会抓老鼠
 *   狗"继承"了动物的所有特征，再增加自己的。
 *
 * 核心手法（内存布局）：
 *   struct dog {
 *       struct animal base;   <-- 把基类嵌进来（"继承"）
 *       void (*guard)();       <-- 自己的扩展
 *   };
 *
 *   内存图：
 *   ┌──────────────────┐  ← dog 的起始地址（也是 base 的起始地址）
 *   │ struct animal {   │
 *   │   name, age       │  ← 继承来的数据
 *   │ }                 │
 *   ├──────────────────┤
 *   │ guard() 函数指针  │  ← 自己新增的数据
 *   └──────────────────┘
 *
 *   因为 base 是第一个成员，所以：
 *     (struct animal *)&dog   ← 安全的"向上转型"（upcasting）
 *     地址完全一样，直接转型即可。
 */

#include <stdio.h>

/****************************************************
 *  基类：所有动物的公共部分
 ****************************************************/
struct animal {
    const char *name;
    int age;
};

/* 通用的动物行为：庆生（所有动物都能用） */
void animal_birthday(struct animal *a)
{
    a->age++;
    printf("%s 过%d岁生日！\n", a->name, a->age);
}

/****************************************************
 *  派生类1：狗 —— 是一种动物
 ****************************************************/
struct dog {
    struct animal base;         /* 继承：嵌入基类（必须放第一个成员） */
    /* 自己的扩展 */
    void (*guard)(struct dog *);
};

static void dog_guard(struct dog *d)
{
    printf("%s: 汪汪！看门中！\n", d->base.name);
}

/* 狗的"构造函数" */
struct dog *dog_create(const char *name, int age)
{
    static struct dog d;
    d.base.name = name;
    d.base.age  = age;
    d.guard     = dog_guard;
    return &d;
}

/****************************************************
 *  派生类2：猫 —— 也是一种动物
 ****************************************************/
struct cat {
    struct animal base;
    void (*hunt)(struct cat *);   /* 猫特有的行为 */
};

static void cat_hunt(struct cat *c)
{
    printf("%s: 抓到老鼠了！\n", c->base.name);
}

struct cat *cat_create(const char *name, int age)
{
    static struct cat c;
    c.base.name = name;
    c.base.age  = age;
    c.hunt      = cat_hunt;
    return &c;
}

int main(void)
{
    struct dog *wangcai = dog_create("旺财", 3);
    struct cat *miaomi  = cat_create("喵喵", 2);

    /*
     * 向上转型（upcasting）：
     *   因为 base 是 dog 的第一个成员，
     *   (struct animal *)wangcai 就是安全的。
     *   不需要知道具体的狗/猫，只要是 animal 就行。
     */
    animal_birthday((struct animal *)wangcai);
    animal_birthday((struct animal *)miaomi);

    /* 特有方法 */
    wangcai->guard(wangcai);
    miaomi->hunt(miaomi);

    return 0;
}

/*
 * ─── 理解检查 ───
 * 1. struct dog 包含 struct animal —— "狗是一种动物"
 * 2. dog 可以调用 animal 的函数（复用代码）
 * 3. dog 还多了自己的 guard —— "扩展"
 * 4. (struct animal *)&dog 是安全的向上转型
 *
 * ─── 动手改改看 ───
 * 1. 在 animal 加一个 weight 字段，dog 和 cat 自动就有
 * 2. 新加一个 duck 派生类（会游泳）
 */
