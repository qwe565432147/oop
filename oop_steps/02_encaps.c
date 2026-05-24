/*
 * 02_encaps.c —— 第2步：封装（Encapsulation）
 *
 * 目标：理解"隐藏内部实现，只暴露接口"是什么意思。
 *
 * 现实类比：
 *   你用遥控器开电视，按一下电源键就行。
 *   ── 你不需要知道电源键背后怎么工作的
 *   ── 这叫"接口"（按电源键 = 开/关）
 *   ── 这叫"封装"（内部电路被塑料壳包住了）
 *
 * 如果没封装：
 *   所有人都可以直接操作内部电路，乱接线就会烧掉电视。
 *
 * 在代码中：
 *   "接口" = .h 文件（告诉你怎么用）
 *   "封装" = .c 文件（你怎么实现的，外部不知道）
 *   本例为了简短，在同一个文件中展示前后的对比。
 */

#include <stdio.h>

/****************************************************
 *  反面教材：没有封装
 *  问题：任何人都可以乱改内部数据
 ****************************************************/
struct bad_dog {
    const char *name;
    int age;
};

/****************************************************
 *  正面教材：有封装
 *  结构体定义对外隐藏，只通过函数操作
 ****************************************************/
struct good_dog;    /* 只告诉你"有这个类型"，不告诉你里面 */

/* 只通过这组函数操作（这就是"接口"） */
struct good_dog *good_dog_create(const char *name, int age);
void good_dog_bark(struct good_dog *d);
int  good_dog_get_age(struct good_dog *d);
void good_dog_set_age(struct good_dog *d, int age);

/* ---- 以下是实现，外部看不到 ---- */
struct good_dog {
    char name[32];      /* 内部实现 */
    int  age;
};

struct good_dog *good_dog_create(const char *name, int age)
{
    static struct good_dog d;   /* 教学简化版，真实用 malloc */
    snprintf(d.name, sizeof(d.name), "%s", name);
    d.age = age;
    return &d;
}

void good_dog_bark(struct good_dog *d)
{
    printf("%s: 汪汪！现在%d岁\n", d->name, d->age);
}

int good_dog_get_age(struct good_dog *d)
{
    return d->age;
}

/* 封装的好处：可以在 set 里加校验逻辑！ */
void good_dog_set_age(struct good_dog *d, int age)
{
    if (age < 0)  age = 0;     /* 不允许负年龄 */
    if (age > 25) age = 25;    /* 狗最多25岁 */
    printf("%s: 年龄从%d改为%d\n", d->name, d->age, age);
    d->age = age;
}

int main(void)
{
    /*
     * 反面：没封装，谁都能乱改
     */
    struct bad_dog bad = {"小黑", 3};
    bad.age = -100;             /* ← 没有保护！不合理的值 */
    printf("没封装：%s %d岁（？？？）\n", bad.name, bad.age);

    /*
     * 正面：有封装，只能通过函数改
     */
    struct good_dog *good = good_dog_create("小黄", 3);
    good_dog_set_age(good, -5);    /* 自动修正为 0 */
    good_dog_set_age(good, 100);   /* 自动修正为 25 */
    good_dog_bark(good);

    /* 不能直接写 good->age = 666 —— 因为结构体定义是隐藏的 */

    return 0;
}

/*
 * ─── 理解检查 ───
 * 1. struct bad_dog：成员暴露，谁都能改，数据不安全
 * 2. struct good_dog：成员隐藏，只能通过函数操作，可以加校验
 * 3. 封装的核心：接口稳定（函数签名不变），实现随便改
 *
 * ─── 动手改改看 ───
 * 1. 给 good_dog_set_age 加一个"只增不减"的限制
 * 2. 加一个 good_dog_get_name 函数
 */
