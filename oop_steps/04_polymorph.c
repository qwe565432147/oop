/*
 * 04_polymorph.c —— 第4步：多态（Polymorphism）
 *
 * 目标：理解"同一个接口调用，不同对象做不同的事"。
 *
 * 这是 OOP 最有威力的特征，也是 Linux 内核驱动模型的核心。
 *
 * 现实类比：
 *   教练喊"跑！" ——>
 *     博尔特：全力冲刺（他是短跑运动员）
 *     马拉松选手：慢跑（他是长跑的）
 *     你：快走（你只是个普通人）
 *   指令都是"跑"，但每个人"跑"的方式不同 = 多态
 *
 *   "我叫一声" ——>
 *     狗：汪汪
 *     猫：喵喵
 *     鸡：咯咯哒
 *   接口都是"叫"，但每个动物叫得不同 = 多态
 *
 * 关键实现：虚表（vtable）
 *   一个结构体装满了函数指针，每个指针指向一个"方法"。
 *   每个对象持有一个指向虚表的指针。
 *
 * 内存结构：
 *   狗对象                         虚表（常量区，所有狗共享）
 *   ┌──────────────┐              ┌──────────────────┐
 *   │ ops ──────────────→          │ .sound = dog_sound│
 *   │ name = "旺财"  │              │ .legs  = 4        │
 *   └──────────────┘              └──────────────────┘
 *
 *   鸡对象                         虚表（另一个地址）
 *   ┌──────────────┐              ┌────────────────────┐
 *   │ ops ──────────────→          │ .sound = chicken_..│
 *   │ name = "咯咯"  │              │ .legs  = 2         │
 *   └──────────────┘              └────────────────────┘
 */

#include <stdio.h>

/* ============== 1. 定义虚表（接口） ============== */

struct animal;  /* 前向声明 */

/* 虚表：定义了"动物能做什么" */
struct animal_ops {
    void (*sound)(struct animal *self);   /* 叫 */
    int  (*legs)(struct animal *self);    /* 几条腿 */
};

/* 基类：每个对象都有一个 ops 指针指向虚表 */
struct animal {
    const struct animal_ops *ops;   /* 虚表指针 */
    const char *name;
};

/* 多态封装函数：通过 ops 间接调用 */
void animal_sound(struct animal *a)
{
    a->ops->sound(a);   /* 跳转到具体实现 */
}

int animal_legs(struct animal *a)
{
    return a->ops->legs(a);
}

/* ============== 2. 具体类型：狗 ============== */

struct dog {
    struct animal base;
    /* 狗特有的成员可以放这里 */
};

static void dog_sound(struct animal *a) { printf("%s: 汪汪！\n", a->name); }
static int  dog_legs(struct animal *a)  { (void)a; return 4; }

/* 虚表实例（static const = 在只读区，所有狗对象共享） */
static const struct animal_ops dog_ops = {
    .sound = dog_sound,
    .legs  = dog_legs,
};

struct dog *dog_create(const char *name)
{
    static struct dog d;
    d.base.ops  = &dog_ops;     /* 绑定狗的虚表 */
    d.base.name = name;
    return &d;
}

/* ============== 3. 具体类型：鸡 ============== */

struct chicken {
    struct animal base;
};

static void chicken_sound(struct animal *a) { printf("%s: 咯咯哒！\n", a->name); }
static int  chicken_legs(struct animal *a) { (void)a; return 2; }

static const struct animal_ops chicken_ops = {
    .sound = chicken_sound,
    .legs  = chicken_legs,
};

struct chicken *chicken_create(const char *name)
{
    static struct chicken c;
    c.base.ops  = &chicken_ops;
    c.base.name = name;
    return &c;
}

/* ============== 4. 具体类型：蛇 ============== */

struct snake {
    struct animal base;
};

static void snake_sound(struct animal *a) { printf("%s: 嘶嘶～\n", a->name); }
static int  snake_legs(struct animal *a)  { (void)a; return 0; }

static const struct animal_ops snake_ops = {
    .sound = snake_sound,
    .legs  = snake_legs,
};

struct snake *snake_create(const char *name)
{
    static struct snake s;
    s.base.ops  = &snake_ops;
    s.base.name = name;
    return &s;
}

/* ============== 5. 使用 ============== */

int main(void)
{
    /*
     * 不同类型，统一放到一个数组里！
     * 这就是多态的威力——不关心具体类型。
     */
    struct animal *zoo[] = {
        (struct animal *)dog_create("旺财"),
        (struct animal *)chicken_create("咯咯"),
        (struct animal *)snake_create("小青"),
    };

    /*
     * 统一遍历：都是 animal_sound()，效果不同！
     * 如果以后加鸭子、加青蛙，这个循环一行不改。
     */
    for (int i = 0; i < 3; i++) {
        printf("%s (%d条腿): ",
               zoo[i]->name, animal_legs(zoo[i]));
        animal_sound(zoo[i]);   /* ← 同一行代码，三个结果 */
    }

    return 0;
}

/*
 * ─── 理解检查 ───
 * 1. struct animal_ops 定义"接口"（动物能做什么）
 * 2. 每个具体类型提供自己的实现（dog_ops / chicken_ops）
 * 3. 每个对象绑定自己的虚表（base.ops = &dog_ops）
 * 4. 调用者通过 ops 间接调用，不关心具体类型
 *
 * ─── 动手改改看 ───
 * 1. 在 animal_ops 里加一个 move() 方法，狗跑、鸡跳、蛇爬
 * 2. 加一个 frog（青蛙），在地上跳、在水里游
 */
