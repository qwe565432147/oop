# C语言面向对象编程 — Linux 内核风格（零基础→精通）

> **适用读者：** 学过 C 基础（变量、指针、结构体、函数），对 Linux 内核好奇的嵌入式开发者。
>
> **学习目标：** 读完 + 跑完 demo，你将有资格说：**"我看得懂 Linux 内核源码了。"**
>
> **版本：** 配套 linux_oop/ 目录中的完整框架代码

---

## 目录

### 第一篇：为什么C语言需要"面向对象"？
- 第1章：一个没有OOP的世界
- 第2章：Linux 内核编码风格——先懂规矩再看代码

### 第二篇：基石——内核C语言的五大兵器
- 第3章：`container_of` —— 内核最关键的宏
- 第4章：侵入式链表 —— `list_head` 详解
- 第5章：虚表 —— `device_ops` / `file_operations`
- 第6章：不透明指针 —— 真正的封装
- 第7章：引用计数 —— `kref` 共享管理

### 第三篇：实战——构建设备模型框架
- 第8章：整体架构设计（从需求到实现）
- 第9章：逐层拆解代码（基类 → 派生类 → 使用）
- 第10章：多态的威力——统一管理所有设备

### 第四篇：内核哲学——模式与禁忌
- 第11章：组合 vs 继承
- 第12章：`goto` 与错误处理
- 第13章：对象生命周期
- 第14章：开闭原则

### 第五篇：从Demo到真正的Linux内核
- 第15章：`file_operations` 实例分析
- 第16章：`kobject` 与 sysfs
- 第17章：驱动模型的 probe/remove
- 第18章：阅读内核源码的路线图

### 附录
- A. 本Demo VS Linux内核 对照表
- B. 常见错误排查
- C. 进阶阅读

---

# 第一篇：为什么C语言需要"面向对象"？

---

## 第1章：一个没有OOP的世界

### 1.1 场景假设

假设你在写嵌入式 firmware，手里有三种灯需要控制：

| 设备 | 通信方式 | 控制方式 |
|------|----------|----------|
| 普通 LED | GPIO 直连 | 写 GPIO 寄存器 |
| RGB LED | I2C 总线 | 发 I2C 命令 |
| 蜂鸣器 | PWM 输出 | 配置 PWM 定时器 |

**没有OOP的世界：**

```c
// 你不得不写这样的代码
void set_device_state(int device_type, int device_id, bool on)
{
    switch (device_type) {
    case TYPE_GPIO_LED:
        write_gpio(device_id, on ? 1 : 0);
        break;
    case TYPE_I2C_RGB:
        i2c_write(device_id, on ? color_data : zero_data);
        break;
    case TYPE_PWM_BUZZER:
        pwm_enable(device_id, on);
        break;
    }
    // 每新增一种设备，就要来这里加 case！
    // 这个函数会随着项目膨胀到无法维护
}
```

**问题出在哪里？**

1. **每次加新设备类型，都要修改 `set_device_state` 函数**——这违反了"开闭原则"（对扩展开放，对修改关闭）
2. **所有设备的知识集中在一个函数里**——高度耦合，牵一发而动全身
3. **想批量操作所有设备**？只能再写一个 `for` 循环+`switch` 的组合
4. **设备相关的数据和操作分离**——每个设备的属性（引脚号、地址等）散落在各处

### 1.2 OOP 的核心思想

OOP 想解决的就是上面这些问题，它的三个核心特征是：

```
封装（Encapsulation）
    把数据 + 操作数据的方法打包在一起，
    对外只暴露接口，隐藏实现细节。
    
继承（Inheritance）
    在已有类型的基础上扩展新类型，
    复用公共代码。

多态（Polymorphism）
    同一个接口，不同的底层实现。
    调用者无需关心具体类型。
```

### 1.3 但C语言没有 class 关键字啊？

没错。但**OOP 是一种思想，不是一种语法**。

C 语言通过以下组合拳，完美实现 OOP 的所有特征：

```
封装   → struct + .h/.c 文件分离 + static 文件作用域
继承   → struct 嵌套（嵌入基类作为第一个成员）
多态   → 函数指针（vtable 模式）
```

**这不只是理论——Linux 内核（3000 万行 C 代码）就是用这套模式构建的。** 它没有用 C++，不是因为内核开发者不会 C++，而是因为 C 的 OOP 模式更可控、更透明、更适合系统编程。

> 记住这句话：**看懂 C 语言的 OOP，你就看懂了 Linux 内核的一半。**

---

## 第2章：Linux 内核编码风格——先懂规矩再看代码

在我们开始写代码之前，必须先理解 Linux 内核的编码风格。你即将看到的代码严格遵守这些规则。

> 为什么要学内核风格？
> 1. 让你能无缝阅读内核源码
> 2. 这些风格都有充分的工程理由（不是随意的审美偏好）

### 2.1 缩进：Tab = 8 字符

```c
/*
 * 内核使用 8 字符宽的 Tab 缩进。
 * 如果你觉得"太宽了"，那是你没看到嵌套超过 3 层的代码。
 * 
 * 8 字符缩进强制你：
 *   1. 不要把代码嵌套太深（超过 3 层就应该重构）
 *   2. 函数足够短小精悍
 *
 * 在 if/for/while 超过 3 层嵌套时，内核开发者会说：
 * "If you need more than 3 levels of indentation,
 *  you're screwed anyway, and should fix your program."
 *    — Linus Torvalds
 */
```

### 2.2 大括号位置

```c
/*
 * 函数：左大括号另起一行
 * 其他（if/while/for）：左大括号在行尾
 */
int function(int x)          /* 函数的大括号在新行 */
{                            /* ← */
        if (x > 0) {         /* if 的大括号在行尾 */
                do_something();
        } else {
                do_other();
        }
}
```

为什么函数和其他语句分开？因为函数定义的行首大括号提供了视觉锚点，方便在源码中快速定位函数边界。

### 2.3 空格规则

```c
/*
 * 关键字后面加空格（if/while/for/switch）
 * 函数名后面不加空格
 */
if (x > 0)          /* if 后面有空格 */
    do_something(x);  /* 函数名后面没空格 */

int ret = function(a, b);  /* 函数名后面没空格 */
```

### 2.4 命名：下划线风格（snake_case）

```c
/*
 * 变量：snake_case
 * 函数：snake_case
 * 宏：大写 + 下划线
 *
 * 不能使用驼峰命名（CamelCase）——C++/Java 的风格。
 * 内核中看到驼峰命名只会出现在：与用户空间交互的 ABI 定义中。
 */
struct device_node *np;     /* node pointer 的缩写 */
unsigned int irq_number;
#define MAX_DEVICES 32
```

### 2.5 注释

```c
/*
 * 多行注释用这种风格
 * 每一行前面都对齐 *
 */

/* 单行注释用这个风格 */

// C++ 风格的单行注释 —— 内核中不允许！
// 因为有些内核编译器的 C 前端不支持这种注释。
// （虽然现在很少见，但内核坚守这个规则）
```

### 2.6 返回值检查

```c
/*
 * 内核风格：
 *   用 if (!ptr) 而不是 if (ptr == NULL)
 *   用 if (ret) 而不是 if (ret != 0)
 *
 * 简洁、直接，不啰嗦
 */
if (!ptr)
    return -ENOMEM;

if (ret)
    return ret;
```

### 2.7 一个完整的内核风格函数

```c
static int gpio_led_probe(struct platform_device *pdev)
{
        struct gpio_led *led;
        int ret;

        led = devm_kzalloc(&pdev->dev, sizeof(*led), GFP_KERNEL);
        if (!led)
                return -ENOMEM;

        led->gpio_num = of_get_gpio(pdev->dev.of_node, 0);
        if (led->gpio_num < 0)
                return led->gpio_num;

        ret = devm_gpio_request(&pdev->dev, led->gpio_num, "led");
        if (ret)
                return ret;

        platform_set_drvdata(pdev, led);
        return 0;
}
```

注意观察：
- Tab 缩进（8字符）
- 函数大括号在新行
- `if (!led)` 而不是 `if (led == NULL)`
- 变量声明在函数开头（C89 风格）
- 错误检查 `if (ret)`
- 函数名和变量名都是 snake_case

> **从现在开始，你可以用这套规范来评判所有 C 代码的质量了。**

---

# 第二篇：基石——内核C语言的五大兵器

---

## 第3章：`container_of` —— 内核最关键的宏

### 3.1 你要解决什么问题？

假设你有两个结构体：

```c
struct device {
    int id;
    char name[32];
};

struct gpio_led {
    struct device base;    /* 嵌入基类 */
    int gpio_num;
    bool state;
};
```

在某段代码中，你只拿到了一个 `struct device *dev` 指针。但你**知道**这个 `dev` 其实是某个 `struct gpio_led` 的 `base` 成员。如何找回原始的 `struct gpio_led *`？

在 C++ 中，你可以用 `dynamic_cast` 或 `static_cast`。在 C 中，你用什么？

### 3.2 offsetof 的原理

`offsetof` 是 `<stddef.h>` 中定义的宏，**计算结构体某个成员相对于结构体起始地址的字节偏移量**。

```c
#include <stddef.h>

struct example {
    char    a;      /* offset 0, 大小 1 */
    int     b;      /* offset 4（对齐到 4）, 大小 4 */
    char    c;      /* offset 8, 大小 1 */
};                  /* 总大小 12（尾部填充到对齐） */

// offsetof(struct example, a) = 0
// offsetof(struct example, b) = 4
// offsetof(struct example, c) = 8
```

**内存布局：**

```
地址:   0   1   2   3   4   5   6   7   8   9   10  11
        ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
        │ a │///│///│///│    b    │ c │///│///│///│///│
        └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
        ↑                        ↑
        &struct                  偏移量 8 (&struct.c)
        = 成员b的地址 - 偏移4    = 成员b地址 + 4
        = 成员c的地址 - 偏移8
```

**offsetof 的标准实现：**

```c
// 标准实现：
#define offsetof(type, member)  ((size_t)&((type *)0)->member)

// 它假想地址 0 处有一个 type 类型的对象，
// 然后取 member 成员的地址，这个地址值正好等于偏移量。
```

> 原理：如果有一个 `struct example` 在地址 0，它的成员 `b` 就在地址 4。所以 `&((struct example *)0)->b) == 4`。

### 3.3 container_of 推导

```c
#define container_of(ptr, type, member)                     \
    ((type *)((char *)(ptr) - offsetof(type, member)))
```

我们一步步拆解：

```
假设：
    dev  是一个 struct device *，它指向某个 GPIO LED 的 base 成员。
    type 是 struct gpio_led
    member 是 base
    
计算过程：
    1. (char *)(ptr)          → 把 dev 的地址转为字节地址（char *）
    2. offsetof(type, member) → base 在 gpio_led 中的偏移量
    3. (char *)(ptr) - 偏移   → 减去偏移量，得到起始地址（在字节层面）
    4. (type *)              → 转回 struct gpio_led *
    
    ======= 类比 =======
    你在一栋楼（gpio_led）里的第3层（base）。
    你站在第3层的走廊（dev = base 的地址）。
    想知道 1 楼大门的位置？
    往楼下走 3 层（减去偏移量）。
    
    container_of 做的就是"往下走 offsetof 层"。
```

**图示：**

```
内存地址  →  struct gpio_led led_object
           ┌──────────────────────────────┐
           │ struct device base {          │  ← 起始地址 (container_of 的结果)
           │     ...                       │
           │ }                             │  ← dev 指针指向这里
           ├──────────────────────────────┤
           │ int gpio_num                  │
           │ bool state                    │
           └──────────────────────────────┘
           
           dev 指向 base ← 我们知道 base 在 gpio_led 中的偏移（假设 0）
           container_of(dev, struct gpio_led, base) = dev 的地址 - 0 = dev 的地址
           
等待，如果 base 是第一个成员（偏移 0），那么 device* 和 gpio_led* 的地址相同。
那 container_of 做了什么？ 
  答：如果 base 是第一个成员，container_of 退化为恒等转换。
      但 container_of 的妙处是：即使 base 不是第一个成员也能工作！
```

### 3.4 为什么不直接用强制类型转换？

```c
// 如果 base 是第一个成员：
struct gpio_led *led = (struct gpio_led *)dev;   // OK，可以工作

// 但如果哪天你在 gpio_led 前面加了另一个成员：
struct gpio_led {
    struct list_head node;    /* 新增的 */
    struct device base;       /* 不再是第一个了！ */
    ...
};

// (struct gpio_led *)dev 现在就错了！！！地址不匹配！！！
// container_of 仍然正确：
struct gpio_led *led = container_of(dev, struct gpio_led, base);  // 永远正确
```

**结论：永远使用 `container_of`，而不是强制类型转换。** 这就是内核代码从不使用 `(struct gpio_led *)dev` 强制转型的原因。

### 3.5 使用场景对照

| 场景 | 不使用 container_of | 使用 container_of |
|------|-------------------|------------------|
| 从 device* → gpio_led* | `(gpio_led *)dev` | `container_of(dev, gpio_led, base)` |
| 从 kref* → device* | 不可靠的类型转换 | `container_of(ref, device, ref)` |
| 从 list_head* → device* | 不可靠的类型转换 | `list_entry(ptr, device, node)` |

---

## 第4章：侵入式链表 —— `list_head` 详解

### 4.1 普通链表 vs 侵入式链表

**普通链表（教科书写法）：**

```c
// 链表节点包含数据
struct list_node {
    int data;               /* 数据 */
    struct list_node *next; /* 指针 */
    struct list_node *prev;
};

// 缺点：
// 1. 每个链表只能存一种数据类型
// 2. 想换个链表？你得重新写一套链表操作
// 3. 链表不知道"数据"的类型，你只能存 int
```

**侵入式链表（内核写法）：**

```c
// 链表节点只有指针
struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

// 数据中包含链表节点
struct gpio_led {
    struct list_head node;  /* 侵入：节点在数据内部 */
    int gpio_num;
    // ...
};

struct i2c_rgb {
    struct list_head node;  /* 同样的 list_head！ */
    uint8_t addr;
    // ...
};

// 优势：
// 1. 同一套链表操作可以管理任意类型
// 2. 一个对象可以同时在多个链表中（多个 list_head 成员）
// 3. 通过 list_entry(ptr, type, member) 获取原对象
```

### 4.2 为什么叫"侵入式"？

因为：你的结构体必须被"侵入"一个 `list_head` 成员。看似是麻烦，实际上给了你**自由**——你的数据不再受链表的约束，而是链表服务于你的数据。

**比喻：**

```
普通链表：酒店的房间号决定了你住哪
         你想换房间？换不了，除非搬行李

侵入式链表：你身上贴了个 RFID 标签
          你可以在任意酒店、任意时间登记
          你甚至可以贴多个标签（同时在多个链表里）
```

### 4.3 链表的初始化与操作

```c
// 方法1：静态声明
LIST_HEAD(my_list);

// 方法2：动态初始化
struct list_head my_list;
INIT_LIST_HEAD(&my_list);

// 空链表就是自环：
//   my_list.next = &my_list;
//   my_list.prev = &my_list;

// 添加元素
list_add(&obj->node, &my_list);       /* 头插 */
list_add_tail(&obj->node, &my_list);  /* 尾插 */

// 删除元素
list_del(&obj->node);

// 判断空
list_empty(&my_list);  /* true 如果 next 指向自己 */
```

### 4.4 `list_for_each_entry` —— 遍历的魔法

这是最常用的遍历宏，用一次你就会爱上它：

```c
struct device *dev;

// 遍历全局设备链表
list_for_each_entry(dev, &device_list, node) {
    printf("%s\n", dev->name);
}
```

**宏展开：**

```c
// 你写的：
list_for_each_entry(dev, &device_list, node) { ... }

// 展开后：
for (dev = list_entry((&device_list)->next, struct device, node);  // 第一个元素
     &dev->node != (&device_list);                                   // 回到头了吗？
     dev = list_entry(dev->node.next, struct device, node))          // 下一个
{ ... }
```

**关键理解：** `list_for_each_entry` 的循环变量（`dev`）不是 `list_head *`，而是**容器结构体指针**（`struct device *`）。它遍历的是数据对象，不是链表节点。这就是侵入式链表的威力——遍历和操作的是**你的数据**，而不是链表的实现细节。

### 4.5 `list_for_each_entry_safe` —— 安全删除

```c
struct device *dev, *tmp;

list_for_each_entry_safe(dev, tmp, &device_list, node) {
    if (should_remove(dev)) {
        list_del(&dev->node);  /* 删除当前节点 */
        /* 没有 tmp 的话，dev->node.next 就变成野指针了 */
    }
}
```

为什么需要 `_safe` 版本？因为 `list_del` 会修改 `dev->node.next` 和 `dev->node.prev`，而 `list_for_each_entry` 的下一次迭代要依赖 `dev->node.next` 来获取下一个元素。预存了 `tmp` 就不怕了。

**类比：** 你在火车上从车头走到车尾，每节车厢都要看一眼。当你拆掉一节车厢（删除），如果你没有提前记住下一节车厢的连接处位置，你就没法继续走了。

---

## 第5章：虚表 —— `device_ops` / `file_operations`

### 5.1 什么是虚表？

虚表（vtable, virtual table）是一个**函数指针结构体**，每个指针指向一个具体的实现：

```c
// "抽象接口"——声明了所有设备都应该支持的操作
struct device_ops {
    int  (*init)(struct device *dev);
    void (*suspend)(struct device *dev);
    void (*resume)(struct device *dev);
    void (*destroy)(struct device *dev);
};
```

不同的设备类型提供不同的实现：

```c
// GPIO LED 的实现
static const struct device_ops gpio_led_ops = {
    .init    = gpio_led_dev_init,
    .destroy = gpio_led_dev_destroy,
    // suspend/resume = NULL（意识是不支持）
};

// I2C RGB 的实现
static const struct device_ops i2c_rgb_ops = {
    .init    = rgb_dev_init,
    .destroy = rgb_dev_destroy,
};
```

调用时通过基类指针间接跳转：

```c
// 只看签名，你能猜出 dev 是哪种设备吗？
int device_init(struct device *dev)
{
    return dev->ops->init(dev);
    //        ↑ 这里会跳到具体的实现
}
```

### 5.2 虚表的内存布局

```
LED 对象                         虚表（VTable，在常量区）
┌──────────────────────┐        ┌───────────────────────┐
│ base.ops ────────────────→    │ .init    = led_init_fn │
│ base.name             │        │ .suspend = NULL        │
│ base.ref              │        │ .resume  = NULL        │
│ base.node             │        │ .destroy = led_del_fn  │
│ gpio_num = 13         │        └───────────────────────┘
│ state = false         │
└──────────────────────┘

RGB 对象                         虚表（不同地址）
┌──────────────────────┐        ┌───────────────────────┐
│ base.ops ────────────────→    │ .init    = rgb_init_fn │
│ base.name             │        │ .suspend = NULL        │
│ ...                   │        │ .resume  = NULL        │
│ addr = 0x22           │        │ .destroy = rgb_del_fn  │
└──────────────────────┘        └───────────────────────┘
```

**关键点：**
- 每个对象有一个 `ops` 指针
- 所有同类型对象共享同一个 `ops` 实例（`static const`）
- `ops` 在常量区（Flash/ROM），不占 RAM
- 这就是"多态"的底层实现

### 5.3 对照 C++ 的虚函数

```cpp
// C++ 写法
class Device {
public:
    virtual int init() = 0;    // 纯虚函数
    virtual ~Device() {}
};

class GPIOLED : public Device {
    int init() override {      // 覆盖
        // do GPIO init
    }
};

// 调用
Device *dev = new GPIOLED();
dev->init();  // 调用的是 GPIOLED::init()
```

```c
// C 的等价写法（我们的框架）
struct device_ops {
    int (*init)(struct device *dev);
};

struct device {
    const struct device_ops *ops;
};

// 调用
struct device *dev = gpio_led_create(...);
dev->ops->init(dev);  // 调用的是 gpio_led 的 init
```

**本质完全一样：** C++ 的虚函数表也是函数指针数组，只是编译器帮你自动生成了。C 让你手动管理——**你失去了方便，但获得了完全的控制**。

---

## 第6章：不透明指针 —— 真正的封装

### 6.1 什么是封装？

拆开来说：**封装 = 信息隐藏**。

使用者只需要知道"怎么用"，不需要知道"怎么实现的"。

```
你调用：led_set_on(led);

你需要知道什么？
    - led 是个指针（类型是 struct gpio_led *）
    - 调用 set_on 就能点亮

你不需要知道什么？
    - gpio_led 结构体有哪些成员
    - GPIO 寄存器地址
    - 当前状态怎么存储的
```

不透明指针（opaque pointer）是 C 语言实现封装的终极手段：**头文件中只声明结构体名字，不暴露成员**。

### 6.2 不透明指针示例

```c
// ---------- gpio_led.h ----------
// 只声明存在这个结构体，不告诉别人里面有什么
struct gpio_led;

// 所有操作通过函数完成
struct gpio_led *gpio_led_create(int pin);
void gpio_led_set_on(struct gpio_led *led);
void gpio_led_set_off(struct gpio_led *led);

// ---------- gpio_led.c ----------
#include "gpio_led.h"

// 结构体的完整定义只在 .c 中
struct gpio_led {
    int pin;
    bool state;
    // ...
};

// 使用结构体内部成员的代码都在 .c 中
void gpio_led_set_on(struct gpio_led *led)
{
    write_pin(led->pin, 1);
    led->state = true;
}
```

### 6.3 为什么内核大量使用不透明指针？

| 原因 | 说明 |
|------|------|
| **ABI 稳定** | 结构体成员变化不影响使用者，不需要重新编译 |
| **安全性** | 使用者无法直接修改内部成员，必须通过接口函数 |
| **解耦** | 头文件的 include 依赖最少，编译更快 |
| **可移植** | 不同平台可以有不同的结构体定义 |

### 6.4 本 Demo 中的封装层次

我们的代码采用了两种封装级别：

**级别1：基类 device 暴露成员（但 ops 是 const）**

```c
// device.h —— 你看到 base 的成员
struct device {
    struct list_head node;
    struct kref ref;
    const char *name;
    const struct device_ops *ops;
};
```

为什么暴露？因为 `container_of` 需要知道成员名才能计算偏移。硬要隐藏也可以，但会牺牲灵活性。

**级别2：派生类（gpio_led 等）的创建函数不暴露结构体布局**

使用者通过 `gpio_led_create()` 获取 `struct device *`，完全不需要知道 `gpio_led` 内部的成员。通过 `to_gpio_led()` 宏向下转型时，只需要在调用的 .c 文件中包含 `gpio_led.h`。

---

## 第7章：引用计数 —— `kref` 共享管理

### 7.1 为什么需要引用计数？

考虑这个场景：

```c
struct device *dev = gpio_led_create("led", 13);
// dev 在被多个模块使用：
module_a_use_device(dev);   // A 模块在用
module_b_use_device(dev);   // B 模块也在用

// 问题：谁来释放 dev？
// 如果 A 先释放了，B 还在用 → 野指针！
// 如果 B 先释放了，A 还在用 → 野指针！
// 等到全都不用的时候再释放？怎么知道"全都不用"了？
```

**引用计数的答案：** 每个使用者 `get()` 一次，用完 `put()` 一次。当计数归零时，最后一个 `put()` 负责释放。

### 7.2 kref 的 API

```c
struct kref {
    unsigned int count;
};

kref_init(k)     // 初始 count = 1（表示一个引用）
kref_get(k)      // count++（新引用）
kref_put(k, release) // count--，如果归零则调用 release()
```

### 7.3 完整生命周期

```
gpio_led_create()
    │
    ├─ malloc()          分配内存
    ├─ kref_init()       count = 1
    ├─ device_register()
    │       └─ ops->init()  硬件初始化
    └─ return dev

...使用过程中...

device_get(dev)    count = 2    ← A 模块拿了引用
device_get(dev)    count = 3    ← B 模块拿了引用
device_put(dev)    count = 2    ← A 用完了
device_put(dev)    count = 1    ← B 用完了

device_unregister(dev)
    ├─ list_del()        从链表移除
    └─ device_put(dev)
            └─ kref_put()
                    ├─ count-- → 0!
                    └─ ops->destroy()
                            └─ free()    ← 真正释放内存
```

**核心规则：** `kref_init` 算一次引用，`device_register` 不额外增加引用。所以 `create` + `unregister` = 一个完整的生命周期。如果中间有人 `get`，就必须对应 `put`，否则不会释放。

---

# 第三篇：实战——构建设备模型框架

---

## 第8章：整体架构设计

### 8.1 需求分析

我们要构建一个"微型设备模型"，需要满足：

1. 支持多种不同类型的设备
2. 提供统一的设备管理接口（注册、查找、遍历）
3. 方便扩展——加新设备不用改现有代码
4. 安全的生命周期管理（不会悬空指针）
5. 代码清晰、可移植（不依赖特定硬件）

### 8.2 架构分层

```
┌──────────────────────────────────────────────────────────┐
│                    应用层（使用者）                        │
│   demo/main.c                                            │
│   └─ 只通过基类接口操作设备，不关心底层实现                │
├──────────────────────────────────────────────────────────┤
│                    设备抽象层                             │
│   include/ktypes.h   — 基础类型 + container_of            │
│   include/klist.h    — 侵入式链表                         │
│   include/kref.h     — 引用计数                           │
│   devices/device.h/c — 基类 + 设备管理核心                │
├──────────────────────────────────────────────────────────┤
│                    具体设备层                             │
│   devices/gpio_led.h/c     — GPIO LED                    │
│   devices/i2c_rgb.h/c      — I2C RGB LED                 │
│   devices/pwm_buzzer.h/c   — PWM 蜂鸣器                  │
└──────────────────────────────────────────────────────────┘
```

### 8.3 文件依赖关系

```
ktypes.h ← container_of, ARRAY_SIZE, 错误码
    ↑
klist.h ← list_head, list_for_each_entry
kref.h ← kref_init/get/put
    ↑
device.h ← struct device, device_ops, 多态接口
device.c ← 设备注册管理、全局链表、device_put
    ↑
gpio_led.h/c, i2c_rgb.h/c, pwm_buzzer.h/c
    ↑
demo/main.c ← 使用示例
```

---

## 第9章：逐层拆解代码

### 9.1 `ktypes.h` — 基础宏

**container_of 宏（最重要！）：**

```c
#define container_of(ptr, type, member)                 \
    ((type *)((char *)(ptr) - offsetof(type, member)))
```

这是我们整个框架的基石。每个具体设备类都通过它从基类指针恢复为具体类型指针。

我们已经在本章第3节详细解释了原理，这里只强调**使用规则**：

```c
// 规则1：ptr 必须指向 type 的 member 成员
//   错误：container_of(dev, struct gpio_led, list) ← dev 不是 list 的成员
//   正确：container_of(dev, struct gpio_led, base) ← OK

// 规则2：确保 ptr 是有效的指针
//   错误：container_of(NULL, struct gpio_led, base) ← 解引用 NULL

// 规则3：确保类型匹配
//   struct device *dev = &led->base;  ← 必须是 &led->base，不能随便传
```

**内核风格错误码：**

```c
#define E_OK     0    /* 成功 */
#define E_PARAM  (-1) /* 参数错误 */
#define E_NOMEM  (-2) /* 内存不足 */
#define E_NODEV  (-3) /* 设备不存在 */
#define E_BUSY   (-4) /* 资源忙 */
```

为什么要用负数？**Linux 内核约定：0 或正数表示成功，负数表示错误。** 为什么？因为正数可以返回成功的数据大小（比如写入的字节数），负数表示错误码。

### 9.2 `klist.h` — 侵入式链表

**核心结构体：**

```c
struct list_head {
    struct list_head *next;
    struct list_head *prev;
};
```

就这么简单。只有两个指针。空链表时指向自己。

**关键函数实现分析：**

```c
// 在两个已知连续节点之间插入
static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = new;
    new->next  = next;
    new->prev  = prev;
    prev->next = new;
}

// 头插：new 被插入到 head 和 head->next 之间
static inline void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}
```

**图解 `list_add`：**

```
操作前：
  head <──> node1 <──> node2
          
操作：list_add(&new, head)
  相当于 __list_add(&new, head, head->next)
                     ↑ new    ↑ prev  ↑ next
                     
  1. next->prev = new → node1->prev = &new
  2. new->next = next → new.next = &node1
  3. new->prev = prev → new.prev = &head
  4. prev->next = new → head->next = &new

操作后：
  head <──> new <──> node1 <──> node2
```

**遍历宏 `list_for_each_entry`：**

```c
#define list_for_each_entry(pos, head, member)                  \
    for (pos = list_entry((head)->next, typeof(*pos), member);  \
         &pos->member != (head);                                 \
         pos = list_entry(pos->member.next, typeof(*pos), member))
```

逐行解读：

1. **初始化：** `pos = list_entry((head)->next, typeof(*pos), member)`
   - 取链表第一个节点的容器对象
   - `typeof(*pos)` 是编译期特性，自动获得 `pos` 指向的类型

2. **条件：** `&pos->member != (head)`
   - 判断是否回到了链表头
   - 空链表时，`head->next == head`，所以条件立即为假

3. **迭代：** `pos = list_entry(pos->member.next, typeof(*pos), member)`
   - 通过当前节点的 `member.next` 找到下一个节点
   - 用 `list_entry`（即 `container_of`）得到下一个容器对象

> **理解 `typeof(*pos)` 为什么是编译期安全的：**
> 在 `for` 循环的初始化部分，`pos` 还没有被赋值，但编译器在编译时就知道 `pos` 是什么类型（比如 `struct device *`），所以 `typeof(*pos)` 就是 `struct device`。这和运行时值无关。

### 9.3 `device.h` — 抽象基类设计

```c
// 前向声明：告诉编译器"struct device 存在，详情后面再说"
struct device;

// 虚函数表：定义所有设备必须支持的操作
struct device_ops {
    int  (*init)(struct device *dev);
    void (*suspend)(struct device *dev);
    void (*resume)(struct device *dev);
    void (*destroy)(struct device *dev);
};

// 基类：所有设备从它派生
struct device {
    struct list_head        node;   /* 链表节点，侵入设备链表 */
    struct kref             ref;    /* 引用计数 */
    const char              *name;  /* 设备名称 */
    const struct device_ops *ops;   /* 虚表指针 → 多态 */
};
```

**设计决策分析：**

| 决策 | 为什么这样设计 | 如果不这样会怎样 |
|------|--------------|----------------|
| `ops` 是 `const *` | 虚表在常量区，所有实例共享 | 每个实例都要复制虚表，浪费 RAM |
| `name` 是 `const char *` | 名称可以是字符串常量 | 需要复制字符串，或自行管理生命周期 |
| `ref` 是嵌入的 `struct kref` | 不需要额外的指针间接访问 | 多一次内存访问 |
| `node` 是嵌入的 `struct list_head` | 设备可以位于链表中 | 无法遍历所有设备 |

**内联多态封装函数：**

```c
static inline int device_init(struct device *dev)
{
    if (!dev || !dev->ops || !dev->ops->init)
        return -E_PARAM;
    return dev->ops->init(dev);
}
```

为什么用 `static inline`？
- `inline` 告诉编译器：把函数体直接嵌入调用处，消除函数调用开销
- `static` 确保每个编译单元有自己的副本，避免链接冲突
- 在头文件中定义函数，让所有使用者都能内联

但注意：这其实不是真正的内联保证——编译器可以忽略 inline 建议。不过好处是降低了调用开销的可能性。

### 9.4 `device.c` — 设备管理核心

**全局设备链表：**

```c
static LIST_HEAD(device_list);
```

`static` 和 `LIST_HEAD` 的组合创造了一个文件作用域的已初始化链表头。外部完全看不到也碰不到 `device_list`。

**`device_register()`：**

```c
int device_register(struct device *dev)
{
    int ret;

    if (!dev || !dev->ops)
        return -E_PARAM;

    if (dev->ops->init) {
        ret = dev->ops->init(dev);
        if (ret) {
            printf("[device] init failed for '%s': err=%d\n",
                   dev->name ? dev->name : "(unnamed)", ret);
            return ret;
        }
    }

    INIT_LIST_HEAD(&dev->node);           /* 初始化链表节点 */
    list_add_tail(&dev->node, &device_list); /* 加入全局链表 */
    printf("[device] registered: %s\n", dev->name);
    return E_OK;
}
```

流程：

```
Create           Register
┌────────┐     ┌──────────────────┐
│ malloc │────→│ 1. ops->init()   │
│ init   │     │ 2. INIT_LIST_HEAD│
│ ref=1  │     │ 3. list_add_tail │
└────────┘     └────────┬─────────┘
                        │
                  [设备已上线，可通过 device_find 找到]
```

**`device_find()`：**

```c
struct device *device_find(const char *name)
{
    struct device *dev;

    if (!name)
        return NULL;

    list_for_each_entry(dev, &device_list, node) {
        if (strcmp(dev->name, name) == 0)
            return dev;
    }
    return NULL;
}
```

遍历链表，逐个比较名称。时间复杂度 O(n)。内核中设备数量通常不多，链表遍历完全够用。

**`device_put()` 和 `device_kref_release()` 的配合：**

```c
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
```

这是整个框架中最精妙的地方：

1. `device_put()` 调用 `kref_put()`，传入 `device_kref_release` 作为回调
2. 当引用计数归零，`kref_put` 调用 `device_kref_release(kref指针)`
3. `device_kref_release` 通过 `container_of` 从 `kref*` 反推出 `device*`
4. 然后调用 `ops->destroy(dev)`，进入具体设备的销毁逻辑
5. 具体设备的 destroy 函数释放内存（`free()`）

**类型转换链：**

```
device_put(struct device *dev)
    ↓
kref_put(struct kref *ref, release_fn)
    ↓  (ref == &dev->ref)
device_kref_release(struct kref *ref)
    ↓  container_of(ref, struct device, ref)
struct device *dev
    ↓  dev->ops->destroy(dev)
gpio_led_dev_destroy(struct device *dev)
    ↓  to_gpio_led(dev)  [container_of]
struct gpio_led *led
    ↓
free(led)
```

**为什么需要 `device_kref_release` 这个中介？** 因为 `kref_put` 的回调签名是 `void (*)(struct kref *)`，但 `ops->destroy` 的签名是 `void (*)(struct device *)`。而 `struct kref` 不是 `struct device` 的第一个成员（前面有 `struct list_head node`），所以不能直接强制转型。用 `container_of` 做一次正确的转换。

### 9.5 `gpio_led` — 第一个具体设备

**头文件 `gpio_led.h`：**

```c
struct gpio_led {
    struct device   base;         /* 继承基类 */
    int             gpio_num;     /* GPIO 引脚 */
    bool            active_high;  /* 高/低有效 */
    bool            state;        /* 当前状态 */
};

/* 向下转型宏 */
static inline struct gpio_led *to_gpio_led(struct device *dev)
{
    return container_of(dev, struct gpio_led, base);
}
```

**关键设计：向下转型宏 `to_gpio_led()`**

内核风格中，每个派生类都有一个 `to_xxx()` 宏或函数，用于从基类指针安全地获取派生类指针。这在 Linux 源码中随处可见：

```c
// 内核真实代码
static inline struct gpio_led *to_gpio_led(struct gpio_led_device *dev)
{
    return container_of(dev, struct gpio_led, dev);
}

// 另一个例子
static inline struct usb_device *to_usb_device(struct device *dev)
{
    return container_of(dev, struct usb_device, dev);
}
```

**实现文件 `gpio_led.c`：**

```c
/* 虚函数实现：具体做什么 */
static int gpio_led_dev_init(struct device *dev)
{
    struct gpio_led *led = to_gpio_led(dev);
    led->state = false;
    printf("[gpio_led] %s: init GPIO_%d (active_%s)\n",
           dev->name, led->gpio_num,
           led->active_high ? "high" : "low");
    return E_OK;
}

static void gpio_led_dev_destroy(struct device *dev)
{
    struct gpio_led *led = to_gpio_led(dev);
    gpio_led_set_off(led);
    printf("[gpio_led] %s: destroy GPIO_%d\n", dev->name, led->gpio_num);
    free(led);    /* 释放 malloc 的内存 */
}

/* 虚函数表（共享的 const 实例） */
static const struct device_ops gpio_led_ops = {
    .init    = gpio_led_dev_init,
    .destroy = gpio_led_dev_destroy,
};

/* 构造函数 */
struct device *gpio_led_create(const char *name,
                               int gpio_num, bool active_high)
{
    struct gpio_led *led = malloc(sizeof(*led));
    if (!led)
        return NULL;

    led->base.ops        = &gpio_led_ops;
    led->base.name       = name;
    led->gpio_num        = gpio_num;
    led->active_high     = active_high;
    led->state           = false;
    kref_init(&led->base.ref);  /* 引用计数 = 1 */

    if (device_register(&led->base)) {
        free(led);
        return NULL;
    }

    return &led->base;
}
```

理解这个文件，你就理解了"如何用 C 实现一个类"：

| C++ 概念 | 本 Demo 的 C 实现 |
|----------|------------------|
| class GPIOLED : public Device | `struct gpio_led { struct device base; ... }` |
| GPIOLED::init() override | `static int gpio_led_dev_init()` |
| virtual table（编译器生成） | `static const struct device_ops gpio_led_ops` |
| new GPIOLED() | `gpio_led_create()` |
| delete led | `gpio_led_dev_destroy()` → `free()` |

### 9.6 `i2c_rgb` 和 `pwm_buzzer` — 更多具体设备

这两个文件和 `gpio_led.c` 的结构完全相同，区别只在于：
- `i2c_rgb` 有 `addr`（I2C 地址）和 `r/g/b`（颜色值）
- `pwm_buzzer` 有 `pwm_pin`、`freq_hz`、`duty_ns`（频率和占空比）

**多态的关键证据：** 三个 `create` 函数都返回 `struct device *`，三套 `ops` 结构体类型相同但绑定不同的函数。对使用者来说，看到的都是 `struct device *`，调用的都是 `device_init(dev)`——但实际执行的代码完全不同。

---

## 第10章：多态的威力——统一管理所有设备

### 10.1 设备数组创建

```c
// 创建三个不同类型的设备，都返回 struct device *
struct device *led   = gpio_led_create("status_led", 13, true);
struct device *rgb   = i2c_rgb_create("rgb_strip", 0x22);
struct device *buz   = pwm_buzzer_create("alert_buzzer", 5);

// 你可以把它们放在同一个数组里！
struct device *devices[] = { led, rgb, buz };
```

### 10.2 统一遍历与操作

```c
// 全部挂起——每个设备执行自己的 suspend 逻辑
for (int i = 0; i < 3; i++)
    device_suspend(devices[i]);

// 全部恢复
for (int i = 0; i < 3; i++)
    device_resume(devices[i]);
```

**这就是多态最直接的应用：同样的函数调用，不同的行为。**

如果不使用多态，你要写：

```c
// 没有多态的版本——重复、容易遗漏、难以维护
gpio_led_suspend(led);
i2c_rgb_suspend(rgb);
pwm_buzzer_suspend(buz);
// 如果忘记添加新设备的 suspend 调用，系统休眠时就会出问题！
```

### 10.3 按名称查找

```c
struct device *dev = device_find("status_led");
if (dev)
    device_suspend(dev);
else
    printf("device not found\n");
```

使用者完全不需要知道 `status_led` 是 GPIO LED 还是 I2C RGB。这就是**多态 + 统一管理**的威力。

### 10.4 回调遍历

```c
static void print_device_info(struct device *dev, void *data)
{
    unsigned int *count = (unsigned int *)data;
    printf("    [%02u] %s\n", (*count)++, dev->name);
}

unsigned int count = 0;
device_for_each(print_device_info, &count);
```

`device_for_each` 把遍历逻辑和使用逻辑分离：
- 框架负责遍历（`device_for_each`）
- 使用者负责处理每个设备（回调函数）

这是"策略模式（Strategy Pattern）"——遍历是一种策略，回调里具体做什么是另一种策略。

### 10.5 从实现到使用：一条完整的链

```
1. gpio_led_create("status_led", 13, true)
   → 分配 struct gpio_led
   → kref_init (count=1)
   → 绑定 ops = &gpio_led_ops
   → device_register
       → ops->init() = gpio_led_dev_init()
       → 加入全局链表

2. device_find("status_led")
   → 遍历全局链表
   → strcmp 匹配 "status_led"
   → 返回 struct device *

3. device_get(dev)
   → kref_get(&dev->ref)  (count++)

4. 使用者调用自定义函数
   struct gpio_led *led = to_gpio_led(dev);  // container_of
   gpio_led_set_on(led);                      // 设备特有方法

5. device_unregister(dev)
   → list_del (从链表移除)
   → device_put
       → kref_put (count--)
       → count = 0? 触发 ops->destroy = gpio_led_dev_destroy
           → free(led) 释放内存
```

---

# 第四篇：内核哲学——模式与禁忌

---

## 第11章：组合 vs 继承

### 11.1 继承解决的问题

继承解决的是 **is-a**（是一种）关系：

```
LED 是一种 设备  →  LED is-a Device
RGB 是一种 设备  →  RGB is-a Device
蜂鸣器 是一种 设备 →  蜂鸣器 is-a Device
```

### 11.2 组合解决的问题

组合解决的是 **has-a**（有一个）关系：

```
设备管理器 有一个 设备链表  →  Manager has-a List
设备对象 有一个 名称       →  Device has-a Name
设备对象 有一个 引用计数   →  Device has-a kref
```

### 11.3 本 Demo 中的应用

**继承（嵌入式基类）：**

```c
struct gpio_led {
    struct device base;     /* 继承：gpio_led 是一种 device */
    int gpio_num;
};
struct i2c_rgb_device {
    struct device base;     /* 继承：i2c_rgb 也是一种 device */
    uint8_t addr;
};
```

**组合（嵌入成员）：**

```c
struct device {
    struct list_head node;  /* 组合：设备"拥有"一个链表节点 */
    struct kref ref;        /* 组合：设备"拥有"一个引用计数器 */
    const char *name;       /* 组合：设备"拥有"一个名称 */
};
```

### 11.4 为什么 Linux 内核更偏爱组合？

Linus Torvalds 本人多次强调：**"继承是万恶之源"**。

内核大量使用组合而不是继承，原因：

1. **继承层次过深会带来脆弱性**——顶层基类的任何变化都会影响所有子类
2. **C 没有访问控制**——protected 成员实际上谁都能访问
3. **多重继承的菱形问题**——C 中可以用多个嵌入模拟，但非常复杂

内核的推荐实践：
- **用组合做"有什么"（has-a）**
- **用虚表做"能做什么"（can-do）**

我们的 `struct device_ops` 就是一个"can-do"的例子：`device` 说"我能被 init、suspend、resume、destroy"，具体怎么做由实现者决定。

---

## 第12章：`goto` 与错误处理

### 12.1 内核风格的错误处理

你可能听说过"goto 有害论"。但在 Linux 内核中，`goto` 是**推荐的错误处理方式**。

### 12.2 没有 goto 的代码

```c
int gpio_led_create_and_setup(void)
{
    struct gpio_led *led = malloc(sizeof(*led));
    if (!led) {
        printf("alloc failed\n");
        return -ENOMEM;
    }

    if (gpio_request(led->gpio_num) != 0) {
        printf("gpio request failed\n");
        free(led);                    // ← 重复释放
        return -EBUSY;
    }

    if (i2c_setup(led->i2c_bus) != 0) {
        printf("i2c setup failed\n");
        gpio_free(led->gpio_num);     // ← 清理步骤遗漏或重复
        free(led);                    // ← 重复释放
        return -EIO;
    }

    return 0;
}
```

问题：随着资源越来越多，每个错误路径都要前向清理，很容易遗漏。

### 12.3 有 goto 的代码（内核风格）

```c
int gpio_led_create_and_setup(void)
{
    struct gpio_led *led;
    int ret;

    /* 分配 */
    led = malloc(sizeof(*led));
    if (!led)
        return -ENOMEM;

    /* 请求 GPIO */
    ret = gpio_request(led->gpio_num);
    if (ret)
        goto err_free_led;

    /* 设置 I2C */
    ret = i2c_setup(led->i2c_bus);
    if (ret)
        goto err_free_gpio;

    return 0;  /* 成功 */

    /* ---- 错误路径（一个出口） ---- */
err_free_gpio:
    gpio_free(led->gpio_num);
err_free_led:
    free(led);
    return ret;  /* 统一返回错误码 */
}
```

**模式总结：**

```
正常路径（从前往后分配资源）：
    alloc(A) → alloc(B) → alloc(C) → return 0

错误路径（从后往前按需释放，goto 标签倒序）：
    err_C: free(C)
    err_B: free(B)
    err_A: free(A)
    return ret;
```

### 12.4 为什么 goto 在这里是好的？

1. **单一出口**——所有错误路径走到同一个结尾，逻辑清晰
2. **避免重复**——释放代码只写一次
3. **层次清晰**——标签顺序和分配顺序正好相反，容易验证

**这不是"滥用 goto"，而是"善用 goto"。** 内核的 goto 只用于错误处理，从不用于向前跳转。

---

## 第13章：对象生命周期

### 13.1 生命周期的四个阶段

```
┌──────────┐     ┌──────────────┐     ┌────────────┐     ┌───────────┐
│  Creation │────→│ Registration │────→│  Usage     │────→│  Destroy  │
│  (分配)   │     │  (注册)      │     │  (使用)    │     │  (释放)   │
└──────────┘     └──────────────┘     └────────────┘     └───────────┘
     │                │                    │                  │
     │ malloc         │ list_add_tail      │ device_find      │ free
     │ kref_init(1)   │ ops->init()        │ device_get/put   │ ops->destroy
     │ ops 赋值       │                    │ to_gpio_led()    │
     └────────────────┴────────────────────┴──────────────────┘
```

### 13.2 封装函数的生命周期对应

```c
// 1. 创建（分配 + 初始化）
struct device *gpio_led_create(...);   // malloc + init + register

// 2. 查找（获取已注册设备的指针）
struct device *device_find("led");     // 遍历链表

// 3. 引用（安全使用）
device_get(dev);                       // 计数 +1
// ... 安全使用 ...
device_put(dev);                       // 计数 -1

// 4. 注销（从系统移除）
device_unregister(dev);                // list_del + put → destroy → free
```

### 13.3 生命周期规则

1. **谁分配，谁释放。** `gpio_led_create` 中的 `malloc` 由 `gpio_led_dev_destroy` 中的 `free` 配对。
2. **`get` 和 `put` 必须成对出现。** 每个 `device_get` 最终需要对应的 `device_put`。
3. **注销后不能再使用。** `device_unregister` 后，指向该设备的指针应立即设为 NULL。
4. **`destroy` 回调不能失败。** 它必须成功清理资源。

### 13.4 一个常见的生命周期 Bug

```c
struct device *dev = gpio_led_create("led", 13, true);

device_get(dev);                    /* count = 2 */
// ... 使用 ...
device_unregister(dev);             /* count = 1 （没有归零！） */
// device_put 匹配之前的 device_get
device_put(dev);                    /* count = 0 → destroy → free */

// 错误！dev 已经被 free，不能再使用！
device_suspend(dev);                /* 野指针！崩溃！ */
```

或者反过来：

```c
struct device *dev = gpio_led_create("led", 13, true);

device_get(dev);                    /* count = 2 */
device_unregister(dev);             /* count = 1 */
// 忘记 device_put!
// 设备永远不会被释放！内存泄漏！
```

**解决方案：** 建立清晰的"所有权"模型。每个模块在取用设备时 `get`，放弃时 `put`。最后一个 `put` 总是由 `device_unregister` 触发。

---

## 第14章：开闭原则

### 14.1 定义

> **对扩展开放（Open for extension）**
> 可以轻松添加新功能
>
> **对修改关闭（Closed for modification）**
> 添加新功能不需要修改现有代码

### 14.2 在 OOP 框架中的体现

假设我们要新增一种设备——`motor_driver`（电机驱动）。

**不改的文件：**
- `include/ktypes.h` ✓ — 基础宏不需要改
- `include/klist.h` ✓ — 链表操作不需要改
- `include/kref.h` ✓ — 引用计数不需要改
- `devices/device.h` ✓ — 基类定义不需要改
- `devices/device.c` ✓ — 设备管理核心不需要改
- `demo/main.c` ✓ 或少量修改 — 如果只是新增，不修改现有逻辑

**需要新建的文件：**
- `devices/motor_driver.h` — 设备特有接口
- `devices/motor_driver.c` — 实现

```c
// motor_driver.h
struct motor_driver {
    struct device base;
    int pwm_pin;
    int dir_pin;
    int speed;
};

static inline struct motor_driver *to_motor(struct device *dev)
{
    return container_of(dev, struct motor_driver, base);
}

struct device *motor_create(const char *name, int pwm_pin, int dir_pin);
void motor_set_speed(struct motor_driver *motor, int speed);

// motor_driver.c
static int motor_dev_init(struct device *dev) { /* ... */ }
static void motor_dev_destroy(struct device *dev) { /* ... */ }

static const struct device_ops motor_ops = {
    .init    = motor_dev_init,
    .destroy = motor_dev_destroy,
};

struct device *motor_create(const char *name, int pwm_pin, int dir_pin)
{
    struct motor_driver *motor = malloc(sizeof(*motor));
    // ... 初始化，注册 ...
    return &motor->base;
}
```

**现在 `motor_driver` 已经可以通过 `device_find` 查找，通过 `device_suspend_all` 批量挂起，通过 `device_for_each` 遍历了——而这些代码一行都没有改动过。**

### 14.3 开闭原则的收益

```
没有 OOP 的扩展：
  修改文件：set_device_state.c, device_type.h, power_manager.c ...
  影响范围：所有调用者
  测试范围：回归测试全部设备

有 OOP 的扩展：
  新建文件：motor_driver.h, motor_driver.c
  影响范围：无（不影响任何现有文件）
  测试范围：只测试新设备
```

这就是为什么说**OOP 降低了软件的维护成本**。

---

# 第五篇：从Demo到真正的Linux内核

---

## 第15章：`file_operations` 实例分析

### 15.1 文件操作是内核最大的多态接口

在 Linux 中，**一切都是文件**。打开一个设备文件（如 `/dev/ttyS0`）、一个 socket、一个普通文本文件，使用的都是 `open()`、`read()`、`write()`、`close()` 这套接口。

但背后实现完全不同——这就是多态。

```c
// Linux 内核 fs.h 中的定义
struct file_operations {
    loff_t (*llseek) (struct file *, loff_t, int);
    ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
    int (*open) (struct inode *, struct file *);
    int (*release) (struct inode *, struct file *);
    unsigned int (*poll) (struct file *, struct poll_table_struct *);
    long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
    int (*mmap) (struct file *, struct vm_area_struct *);
    // ... 还有很多
};
```

### 15.2 对照本 Demo

| 本 Demo | Linux 内核 |
|---------|-----------|
| `struct device_ops` | `struct file_operations` |
| `device_init(dev)` | `file->f_op->open(inode, file)` |
| `device_suspend(dev)` | `dev->pm->suspend(dev)` |
| 虚表通过 ops 指针访问 | `file->f_op` 指向具体文件系统的 ops |
| 使用者不关心具体类型 | 调用者不知道文件在哪个文件系统上 |

### 15.3 一个具体的例子

```c
// ext4 文件系统的 read 实现
const struct file_operations ext4_file_operations = {
    .read_iter    = ext4_file_read_iter,
    .write_iter   = ext4_file_write_iter,
    .open         = ext4_file_open,
    .release      = ext4_file_release,
    .mmap         = ext4_file_mmap,
    // ...
};

// socket 的 read 实现
const struct file_operations socket_file_ops = {
    .read_iter    = sock_read_iter,
    .write_iter   = sock_write_iter,
    .open         = sock_no_open,     // socket 不支持 open
    .release      = sock_close,
    .poll         = sock_poll,
    // ...
};

// 调用时：
// file->f_op->read_iter(...) 会自动分派到正确的实现
// ext4 文件 → ext4_file_read_iter
// socket   → sock_read_iter
```

**一模一样的设计模式**，只是应用场景不同。

---

## 第16章：`kobject` 与 sysfs

### 16.1 内核的设备模型层次

```
struct kobject   ← 最基础的"对象"
    ↑
struct device   ← 设备对象（继承 kobject）
    ↑
struct gpio_led ← 具体设备（继承 device）
```

### 16.2 kobject 的功能

`kobject` 提供了：
1. **引用计数**（kref）
2. **名称**（name）
3. **父子关系**（parent）
4. **sysfs 表示**（在 `/sys` 下创建目录）

本 Demo 的 `struct device` 就是 `kobject` 的简化版——去掉了 sysfs 相关部分，保留了核心功能。

### 16.3 container_of 在 kobject 系统中的应用

```c
// 从 kobject * 得到 container device *
struct device *kobj_to_dev(struct kobject *kobj)
{
    return container_of(kobj, struct device, kobj);
}
```

**和我们的 `to_gpio_led()` 完全相同的手法。**

---

## 第17章：驱动模型的 probe/remove

### 17.1 内核的驱动生命周期

```c
// 当内核发现一个设备时，调用 probe
static int gpio_led_probe(struct platform_device *pdev)
{
    struct gpio_led *led;

    led = devm_kzalloc(&pdev->dev, sizeof(*led), GFP_KERNEL);
    if (!led)
        return -ENOMEM;

    /* 初始化硬件（类似我们的 ops->init） */
    platform_set_drvdata(pdev, led);
    return 0;
}

// 当内核移除设备时，调用 remove
static int gpio_led_remove(struct platform_device *pdev)
{
    struct gpio_led *led = platform_get_drvdata(pdev);
    /* 清理（类似我们的 ops->destroy） */
    return 0;
}

// 驱动注册 —— 类似于我们的 device_register
static struct platform_driver gpio_led_driver = {
    .probe  = gpio_led_probe,
    .remove = gpio_led_remove,
    .driver = {
        .name = "gpio-led",
    },
};
```

### 17.2 对照本 Demo

| 本 Demo | Linux 内核驱动模型 |
|---------|------------------|
| `gpio_led_create()` | `probe()` 函数 |
| `device_register()` | `platform_driver_register()` |
| `ops->init()` | `probe()` 中的初始化 |
| `device_find()` | `bus_find_device()` |
| `device_put()` | `put_device()` |
| `ops->destroy()` + `free()` | `remove()` + `devm_kfree()` |

---

## 第18章：阅读内核源码的路线图

### 18.1 推荐阅读顺序

掌握了本 Demo 后，你可以按以下顺序深入内核源码：

```
第一阶段：巩固基础
    1. include/linux/list.h          ← list_head 完整实现
    2. include/linux/kref.h          ← kref 完整实现
    3. include/linux/kernel.h        ← container_of, ARRAY_SIZE 等

第二阶段：理解设备模型
    4. include/linux/kobject.h       ← kobject 基础
    5. include/linux/device.h        ← 内核的 struct device
    6. drivers/base/core.c           ← device_register 实现

第三阶段：看具体驱动
    7. drivers/leds/leds-gpio.c      ← GPIO LED 驱动
    8. drivers/i2c/i2c-core-base.c   ← I2C 核心
    9. drivers/pwm/pwm-core.c        ← PWM 框架

第四阶段：文件系统
    10. include/linux/fs.h           ← file_operations
    11. fs/ext4/file.c               ← ext4 实现
```

### 18.2 推荐工具

- **cscope**：源码索引，快速跳转
- **Code Browser**：图形化源码查看
- **printk 调试**：内核版的 printf
- **qemu + gdb**：单步调试内核

---

# 附录 A：本 Demo VS Linux 内核 对照表

| 概念 | 本 Demo | Linux 内核 |
|------|---------|-----------|
| container_of | `include/ktypes.h` | `include/linux/kernel.h` |
| 侵入式链表 | `include/klist.h` | `include/linux/list.h` |
| 引用计数 | `include/kref.h` | `include/linux/kref.h` |
| 基类对象 | `struct device` | `struct kobject` / `struct device` |
| 虚函数表 | `struct device_ops` | `struct file_operations` / `struct bus_type` |
| 继承方式 | `struct gpio_led { struct device base; ... }` | `struct gpio_led { struct gpio_chip gc; ... }` |
| 向下转型 | `to_gpio_led(dev)` | `to_gpio_chip(gc)` / `to_usb_device(dev)` |
| 设备注册 | `device_register()` | `device_register()` / `platform_driver_register()` |
| 设备查找 | `device_find(name)` | `bus_find_device()` / `driver_find()` |
| 引用操作 | `device_get() / device_put()` | `get_device() / put_device()` |
| 错误码 | `E_OK, E_PARAM, E_NOMEM...` | `0, -EINVAL, -ENOMEM, -EBUSY...` |
| 构造函数 | `xxx_create()` → 返回 `struct device *` | `probe()` → 填充驱动私有数据 |
| 遍历回调 | `device_for_each(cb, data)` | `bus_for_each_dev(bus, NULL, data, fn)` |

---

# 附录 B：常见错误排查

### B.1 编译错误

**"implicit declaration of function 'typeof'"**
- 原因：用了 `-std=c99` 但 `typeof` 是 GNU 扩展
- 解决：使用 `-std=gnu99` 替代 `-std=c99`

**"'struct device' declared inside parameter list"**
- 原因：在 `struct device` 定义之前就使用了它作为参数类型
- 解决：在前面加上 `struct device;` 前向声明

**"dereferencing pointer to incomplete type"**
- 原因：试图访问只前向声明、未完整定义的结构体成员
- 解决：检查 `.c` 文件是否包含了定义该结构体的头文件

### B.2 运行时错误

**"Segmentation fault / 段错误"**
- 最常见原因：指针为 NULL 但未检查就访问成员
- 解决方法：
  - 每次使用指针前检查 `if (!ptr) return -E_PARAM;`
  - 使用 `device_find` 后判断返回值是否为 NULL

**"double free / double destroy"**
- 原因：`device_unregister` 被调用了两次
- 解决方法：`device_unregister` 后把指针设为 NULL

**"设备从未被释放（内存泄漏）"**
- 原因：`device_get` 和 `device_put` 数量不匹配
- 解决方法：检查每个 `get` 是否都有对应的 `put`

---

# 附录 C：进阶阅读

### 书籍
1. **《Linux 内核设计与实现》Robert Love** — 入门最佳，尤其是第 2 章（内核开发基础）
2. **《深入理解 Linux 内核》Daniel Bovet** — 第三版以上，涵盖设备模型
3. **《Linux Device Drivers》第三版** — 经典驱动开发手册（虽然基于 2.6 内核，核心思想未变）
4. **《C 专家编程》**——帮助你理解 C 语言的底层细节

### 内核源码快速索引
- [include/linux/list.h](https://elixir.bootlin.com/linux/latest/source/include/linux/list.h) — list_head
- [include/linux/kref.h](https://elixir.bootlin.com/linux/latest/source/include/linux/kref.h) — kref
- [include/linux/device.h](https://elixir.bootlin.com/linux/latest/source/include/linux/device.h) — 设备模型
- [include/linux/fs.h](https://elixir.bootlin.com/linux/latest/source/include/linux/fs.h) — file_operations
- [include/linux/kobject.h](https://elixir.bootlin.com/linux/latest/source/include/linux/kobject.h) — kobject

### 在线资源
- [Bootlin Elixir Cross-Referencer](https://elixir.bootlin.com/linux/latest/source) — 在线源码浏览器
- [Linux Kernel Newbies](https://kernelnewbies.org/) — 内核新手社区
- [LWN.net](https://lwn.net/) — 内核开发新闻和深入分析

---

> **后记：**
>
> 恭喜你读到了这里！如果你真的逐行读了代码、跑了 demo、思考了每章的问题，那么你已经具备了阅读 Linux 内核源码的基本能力。
>
> 记住：Linux 内核也是一个 C 程序。它的"神秘感"只来自于不知道那些设计模式。现在你已经知道了——`container_of`、`list_head`、`file_operations`、`kref`、`device`——这些构成了内核的骨架。
>
> 去读源码吧，它比你想象的简单。
>
> 下一步：打开 [include/linux/list.h](https://elixir.bootlin.com/linux/latest/source/include/linux/list.h)，看看真正的内核链表实现。你会发现，和我们写的 klist.h 几乎一样。
