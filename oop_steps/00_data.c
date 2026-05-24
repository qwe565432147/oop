/*
 * 00_data.c —— 第0步：结构体 = 对象的数据
 *
 * 目标：理解"对象"其实就是"数据 + 行为"的打包。
 *       本文件先只做"数据"部分。
 *
 * Python中的类：
 *   class Dog:
 *       def __init__(self, name, age):
 *           self.name = name    # 数据
 *           self.age  = age     # 数据
 *
 *   d = Dog("旺财", 3)
 *   print(d.name)   # 访问数据
 *
 * C语言中的结构体 = Python类的"数据部分"：
 *   把一组相关的变量打包在一起，这就是"对象"的雏形。
 */

#include <stdio.h>

/* 定义一个"类型"——就像 Python 的 class Dog: */
struct dog {
    const char *name;   /* 属性 */
    int age;            /* 属性 */
};

int main(void)
{
    /* "实例化"——就像 Python 的 Dog("旺财", 3) */
    struct dog wangcai = {"旺财", 3};
    struct dog laifu   = {"来福", 5};

    /* 访问属性——就像 Python 的 wangcai.name */
    printf("name = %s, age = %d\n", wangcai.name, wangcai.age);
    printf("name = %s, age = %d\n", laifu.name, laifu.age);

    /* 动手试试：加一个品种字段 breed，再创建一只狗 */
    return 0;
}

/*
 * ─── 理解检查 ───
 * 1. struct dog 相当于 Python 的 class 定义
 * 2. struct dog wangcai 相当于 wangcai = Dog()
 * 3. wangcai.name 完全一样！
 * 4. 区别：C的结构体只能装数据，不能装方法（函数）
 *
 * 但别急——下个文件就让结构体能"装方法"。
 */
