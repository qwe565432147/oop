# C语言面向对象编程 — 从零开始学（嵌入式实战）

> **适用读者：** 学过 C 语言基础（变量、函数、指针、结构体），想理解面向对象思想以及如何在 C 中实现它的嵌入式开发者。

---

## 目录

1. [到底什么是「面向对象」？](#1-到底什么是面向对象)
2. [结构体就是对象的雏形](#2-结构体就是对象的雏形)
3. [函数指针——把函数也变成数据](#3-函数指针把函数也变成数据)
4. [封装——把实现藏起来](#4-封装把实现藏起来)
5. [继承——"是一种" 的关系](#5-继承是一种的关系)
6. [多态——一个接口，多种行为](#6-多态一个接口多种行为)
7. [方法调用的两种风格](#7-方法调用的两种风格)
8. [扩展一个新的灯型](#8-扩展一个新的灯型)
9. [完整代码逐行解读](#9-完整代码逐行解读)
10. [总结与进阶建议](#10-总结与进阶建议)

---

## 1. 到底什么是「面向对象」？

### 1.1 生活中的例子

想象你是开发板上的 LED 灯。我说"点亮"，不同的灯有不同的做法：

| 灯的类型 | "点亮"的含义 |
|---|---|
| GPIO 直控 LED | 把引脚电平拉高/拉低 |
| I2C RGB 灯 | 通过 I2C 总线发送颜色数据 |
| WS2812 彩灯 | 通过单总线协议发送 24bit 数据 |

但作为**使用者**，我不想知道这些细节。我只想说一句 **"开灯"**，灯就亮了。

面向对象要解决的核心问题就是：**对外统一接口，对内各干各的**。

### 1.2 面向对象三大特征

| 特征 | 通俗理解 | 类比 |
|---|---|---|
| **封装** | 把数据和操作数据的函数打包在一起，对外只暴露接口 | 遥控器：你按按钮就行，不用管里面电路 |
| **继承** | 在已有类的基础上扩展新类 | 普通 LED → RGB LED（能变色），普通 LED 的功能 RGB 都有 |
| **多态** | 同一个接口，不同实现 | 都是 `开灯()`，GPIO 灯写引脚，RGB 灯发 I2C |

### 1.3 C语言能做OOP吗？

C++/Java/Python 这些语言原生支持 OOP 语法。C 语言没有 `class`、`virtual` 关键字，但**OOP 是一种思想，不是一种语法**。用 C 的结构体 + 函数指针完全能实现同样的效果——很多嵌入式项目和操作系统（如 Linux 内核）就是这么做的。

---

## 2. 结构体就是对象的雏形

回想一下，C 语言里怎么把多个数据打包在一起？

```c
// 一个灯对象：它有"属性"
struct LED {
    int  pin;          // 引脚号
    bool active_high;  // 高/低电平点亮
    bool state;        // 当前状态
};
```

这就是最原始的对象——它包含了描述一个 LED 需要的所有**数据**。

但对象不仅有数据，还要有**行为**（函数）。如果我们把操作这个 LED 的函数也跟它绑定在一起：

```c
struct LED {
    int  pin;
    bool active_high;
    bool state;

    // 把函数指针也作为成员
    void (*set_on)(struct LED *self);
    void (*set_off)(struct LED *self);
};
```

现在 `struct LED` 既有数据又有方法了！这就很像一个 **"类"**。

> **💡 关键理解：面向对象就是把「数据」和操作这些数据的「函数」绑定在一起，形成一个整体。**

---

## 3. 函数指针——把函数也变成数据

### 3.1 什么是函数指针？

就像 `int *p` 存了一个整数的地址一样，函数指针存的是函数的地址。

```c
int add(int a, int b) { return a + b; }

int (*fp)(int, int);   // 声明一个函数指针
fp = add;              // 指向 add 函数
int result = fp(3, 4); // 通过指针调用函数
```

### 3.2 函数指针作为结构体成员

这是实现 OOP 的关键技术。结构体的成员可以是**数据**也可以是**函数指针**：

```c
// "方法表" —— 把一组相关函数指针打包
struct Methods {
    void (*set_on)(struct LED *self);
    void (*set_off)(struct LED *self); 
};

// 每个 LED 对象都有自己的方法表
struct LED {
    struct Methods fn;   // ← 函数指针作为成员
    int pin;
    bool active_high;
};
```

初始化时把具体的函数地址填进去：

```c
// 实际点亮函数
void gpio_set_on(struct LED *self) {
    write_pin(self->pin, 1);
}

// 初始化：绑定函数指针
void led_init(struct LED *self, int pin) {
    self->fn.set_on  = gpio_set_on;   // 绑定
    self->fn.set_off = gpio_set_off;
    self->pin = pin;
}
```

调用时就像 Python 的 `对象.方法()`：

```c
struct LED my_led;
led_init(&my_led, 13);
my_led.fn.set_on(&my_led);   // ← 对象.方法(对象) 风格
```

> **⚠️ 代价：** 每个 LED 实例都要存一套函数指针（5个指针 ≈ 20-40 字节）。对于只有几个灯的应用来说完全不是问题。如果数量极大，可以用共享虚表的方式节省内存（见第7章）。

---

## 4. 封装——把实现藏起来

### 4.1 什么是封装？

封装的意思是：**公开接口，隐藏细节**。

就跟你用手机一样——你只需要知道按开机键能开机，不需要知道电源管理芯片是怎么工作的。

在代码上的体现就是：

- `.h` 文件 = **公共接口**（告诉别人你可以怎么用）
- `.c` 文件 = **私有实现**（具体怎么做，外人看不见）

### 4.2 在头文件中只暴露操作函数

```c
/* led.h — 公共接口 */

// 你只需要知道这个结构体存在，具体成员可以不看
typedef struct LED LED;

// 你只需要知道这两个函数
void led_init(LED *self, int pin, bool active_high);
void led_deinit(LED *self);
```

而 `.c` 文件里实现这些函数。外部用户**根本不需要知道 GPIO 是怎么操作的**。

### 4.3 为什么封装很重要？

```
没有封装：
    每个使用者都要写：
        if (灯是高电平有效) {
            write_pin(13, 1);  // 点亮
        } else {
            write_pin(13, 0);  // 点亮
        }
    如果改了引脚或换了灯型，所有地方都要改！

有封装：
    使用者只需要写：
        light_on(&my_led);
    
    底层实现变化了，使用者代码一个字都不用改！
```

封装的核心价值：**把变化关在笼子里**。

---

## 5. 继承——"是一种" 的关系

### 5.1 为什么要继承？

我们有一类东西叫"灯"，它们有一些共性（都能开/关/调亮度）。又有很多种具体的灯（普通 LED、RGB 灯、WS2812……）。

继承让我们可以：
1. **先定义通用的接口**（灯应该能做什么）
2. **再分别实现具体的灯**（不同的灯怎么完成这些操作）

### 5.2 C 语言如何实现继承

C 没有 `class LED : public Light` 语法，但有个简单的技巧——把基类作为派生类的**第一个成员**：

```c
// 基类：所有灯的公共部分
struct Light {
    const struct LightOps *ops;   // 虚表指针
};

// 派生类：LED "是一种" Light
struct LED {
    struct Light base;      // ← 第一个成员放基类
    int  pin;
    bool active_high;
    bool state;
};
```

### 5.3 内存布局

```
内存地址 →  LED 对象
          ┌─────────────┐
          │ ops (指针)   │ ← 这部分就是 Light（基类数据）
          ├─────────────┤
          │ fn.set_on   │
          │ fn.set_off  │
          │ ...         │ ← 这部分是 LED 自己的数据
          ├─────────────┤
          │ pin         │
          │ active_high │
          │ state       │
          └─────────────┘
```

因为基类在开头，所以 `(Light *)&led` 转换后指针位置不变，完全合法。这就是 **"向上转型"**（upcasting）。

```c
struct LED my_led;
struct Light *p = (struct Light *)&my_led;  // OK，安全的向上转型
```

### 5.4 "继承"在代码中的含义

继承描述的是 **is-a**（是一种）关系：
- LED **是一种** Light
- RGBLight **也是一种** Light

所以在任何需要 `Light*` 的地方，你都可以传入 `LED*` 或 `RGBLight*`。

---

## 6. 多态——一个接口，多种行为

### 6.1 什么是多态？

多态的意思是 **"相同的调用，不同的行为"**。

```c
Light *lights[2];
lights[0] = (Light *)&my_led;
lights[1] = (Light *)&my_rgb;

light_on(lights[0]);   // 调用 LED 的 set_on（写GPIO）
light_on(lights[1]);   // 调用 RGB 的 set_on（发I2C）
```

**同样的 `light_on()`，执行了不同的操作。** 这就是多态。

### 6.2 虚表（vtable）原理

关键在 `LightOps`（虚函数表）：

```c
// 虚函数表类型定义
typedef struct {
    void (*set_on)(Light *self);
    void (*set_off)(Light *self);
    void (*toggle)(Light *self);
    void (*set_brightness)(Light *self, uint8_t);
    bool (*is_on)(Light *self);
} LightOps;
```

每个具体的灯类型提供一份自己的实现表：

```c
// LED 的虚表
static const LightOps led_ops = {
    .set_on  = led_set_on,     // → 写 GPIO 寄存器
    .set_off = led_set_off,
    // ...
};

// RGB 灯的虚表
static const LightOps rgb_ops = {
    .set_on  = rgb_set_on,     // → 发 I2C 命令
    .set_off = rgb_set_off,
    // ...
};
```

对象初始化时绑定相应的虚表：

```c
void led_init(LED *self, ...) {
    self->base.ops = &led_ops;   // 绑定 LED 的虚表
    // ...
}

void rgb_light_init(RGBLight *self, ...) {
    self->base.ops = &rgb_ops;   // 绑定 RGB 的虚表
    // ...
}
```

### 6.3 调用过程图解

```
你写： light_on(p);
              │
              ▼
    light_on() 是 inline 函数（light.h中定义）：
    ┌─────────────────────────────┐
    │ p->ops->set_on(p)           │  ← 通过 ops 指针跳转
    └─────────────────────────────┘
              │
              ▼
        ┌─────┴─────┐
        │           │
   p->ops =    p->ops =
  &led_ops    &rgb_ops
        │           │
        ▼           ▼
  led_set_on()  rgb_set_on()
  (写GPIO)     (发I2C)
```

### 6.4 为什么需要多态？

没有多态，你需要自己判断类型并分别处理：

```c
// 没有多态 —— 又丑又难维护
if (type == LED_TYPE) {
    led_set_on(&led);
} else if (type == RGB_TYPE) {
    rgb_set_on(&rgb);
} else if (type == WS2812_TYPE) {
    ws2812_set_on(&ws);
}
```

有多态，统一处理：

```c
// 有多态 —— 简洁优雅
for (int i = 0; i < NUM_LIGHTS; i++) {
    light_on(lights[i]);   // 具体怎么执行？灯自己知道
}
```

---

## 7. 方法调用的两种风格

本 demo 实现了两种调用风格，你可以根据场景选择。

### 7.1 风格一：`对象.方法(对象)` Python 风格

```c
// 直接通过对象的 fn 方法表调用
led_status.fn.set_on(&led_status);    // 像 Python 的 led.on()
rgb_strip.fn.set_on(&rgb_strip);
```

**特点：**
- 代码阅读顺序和自然语言一致：**"灯.开(这盏灯)"**
- 每个对象内部有一份独立的方法表
- 编译时就确定了调用哪个函数（没有运行时开销）
- 代价：每个对象多占几个指针的内存

**原理：**

```c
// LED 结构体中有个 fn 成员
struct LED {
    Light     base;
    LED_Fn    fn;          // ← 函数指针结构体

    int       pin;
    bool      active_high;
    bool      state;
};

// fn 的类型：
typedef struct {
    void (*set_on)(LED *self);         // ← 函数指针，参数是 LED*
    void (*set_off)(LED *self);
    void (*toggle)(LED *self);
    void (*set_brightness)(LED *self, uint8_t);
    bool (*is_on)(LED *self);
} LED_Fn;
```

为什么 `set_on` 的参数是 `LED*` 而不是 `Light*`？因为通过 `fn` 调用时，我们已经确定操作的是 LED 类型，不需要多态。

### 7.2 风格二：`接口名_方法(对象)` 多态风格

```c
// 通过基类指针和虚表调用
Light *p = (Light *)&led_status;
light_on(p);              // 运行期自动派发
light_off(p);
```

**特点：**
- 可以用数组统一管理不同类型对象
- 运行期动态分发，扩展新灯型无需改上层代码
- 虚表在常量区，所有同类实例共享
- 多了一层函数跳转，但编译器通常能优化掉

### 7.3 两种风格的对比

| | `obj.fn.method(obj)` | `light_method(obj)` |
|---|---|---|
| 写法 | `led.fn.set_on(&led)` | `light_on(p)` |
| 派发时机 | 编译期 | 运行期 |
| 内存 | 每实例一套函数指针 | 共享虚表 |
| 类型安全 | 强（参数是具体类型） | 弱（参数是基类） |
| 适合场景 | 明确知道具体类型 | 需要统一管理多种类型 |

### 7.4 两种风格在同一个模块中并存

观察 `led.c` 会发现，两种调用风格的实现函数是**同一组**：

- `led_set_on(LED *self)` — 真正的实现
- 这个函数被绑定到 `fn.set_on`（供 Python 风格调用）
- 这个函数也被包装成 `LightOps` 签名（`void (*)(Light*)`），供多态调用

所以选择哪种风格只是**调用端的偏好**，底层是一样的。

---

## 8. 扩展一个新的灯型

假设我们有一个 WS2812 智能彩灯（单总线协议），想加入系统。

### 8.1 创建头文件

```c
/* ws2812_light.h */
#pragma once
#include "light.h"

typedef struct WS2812 WS2812;

typedef struct {
    void (*set_on)(WS2812 *self);
    // ... 其他方法
} WS2812_Fn;

struct WS2812 {
    Light        base;
    WS2812_Fn    fn;
    int          pin;       /* 数据引脚 */
    uint8_t      r, g, b;   /* 当前颜色 */
    bool         state;
};

void ws2812_init(WS2812 *self, int pin);
void ws2812_set_color(WS2812 *self, uint8_t r, uint8_t g, uint8_t b);
```

### 8.2 创建实现文件

```c
/* ws2812_light.c */
#include "ws2812_light.h"
#include <stdio.h>

static void ws_set_on(WS2812 *self) {
    self->state = true;
    printf("[WS2812] GPIO_%d send %u %u %u\n", self->pin, self->r, self->g, self->b);
}

// ... 其他实现函数

/* 绑定到 fn 方法表 */
void ws2812_init(WS2812 *self, int pin) {
    self->fn.set_on  = ws_set_on;
    // ...
    self->base.ops = &ws2812_ops;   /* 绑定到多态虚表 */
}
```

### 8.3 上层代码不做任何改动

```c
// 老的灯不受影响
light_on(lights[0]);   // LED
light_on(lights[1]);   // RGB

// 新灯直接使用
WS2812 ws;
ws2812_init(&ws, 4);
ws.fn.set_on(&ws);     // Python 风格
light_on((Light *)&ws); // 多态风格（因为绑定了 ws2812_ops）
```

这个过程中 **没有修改任何已有的代码**。这就是 OOP 的威力——**对扩展开放，对修改关闭**（开闭原则）。

---

## 9. 完整代码逐行解读

这里逐文件解释关键部分的设计思路。

### 9.1 `light.h` — 抽象基类

```c
// LightOps：虚函数表 —— 定义所有灯都应该有的操作
typedef struct {
    void     (*set_on)(Light *self);
    void     (*set_off)(Light *self);
    void     (*toggle)(Light *self);
    void     (*set_brightness)(Light *self, uint8_t brightness);
    bool     (*is_on)(Light *self);
} LightOps;

// Light 类：只有一个成员 —— 指向虚表的指针
struct Light {
    const LightOps *ops;
};
```

**这就像 Java 的接口（Interface）或 C++ 的抽象类。** 它只定义了"灯应该做什么"，不规定"怎么做"。

`inline` 封装函数是对外的主要调用入口：

```c
static inline void light_on(Light *self) {
    self->ops->set_on(self);   // 通过虚表跳转到具体实现
}
```

`inline` 的意思是：编译时直接把函数体"粘贴"到调用处，消除函数调用开销。

### 9.2 `led.h` — LED 头文件

```c
// 方法表类型定义
typedef struct {
    void (*set_on)(LED *self);         // 注意参数是 LED*，不是 Light*
    void (*set_off)(LED *self);
    // ...
} LED_Fn;

struct LED {
    Light     base;     // 继承 Light（是第一个成员）
    LED_Fn    fn;       // 方法表（支持 obj.fn.method() 风格）
    int       pin;      // 属性：引脚号
    bool      active_high;  // 属性：高/低电平有效
    bool      state;    // 属性：当前状态
};
```

**继承体现在 `Light base` 作为第一个成员。** 封装体现在 `.h` 只暴露 `led_init` / `led_deinit`。

### 9.3 `led.c` — LED 实现文件

```c
/* 真正干活的函数：static 意味着外部看不到 */
static void led_set_on(LED *self) {
    self->state = true;
    int level = self->active_high ? 1 : 0;
    write_pin(self->pin, level);  // 实际操作硬件
}
```

多态适配器：因为 `LightOps` 里的函数签名是 `void (*)(Light*)`，需要先转型：

```c
static void led_ops_set_on(Light *base) {
    led_set_on((LED *)base);  // Light* → LED* 向下转型
}
```

虚表是 `static const` 的，放在常量区，所有 LED 实例共享：

```c
static const LightOps led_ops = {
    .set_on  = led_ops_set_on,
    .set_off = led_ops_set_off,
    // ...
};
```

初始化函数同时绑定两种调用方式：

```c
void led_init(LED *self, int pin, bool active_high) {
    self->base.ops = &led_ops;       // 绑定虚表（多态用）
    self->fn.set_on = led_set_on;    // 绑定方法表（Python风格用）
    self->fn.set_off = led_set_off;
    // ...
    self->pin = pin;
    self->state = false;
}
```

### 9.4 `main.c` — 使用示例

```c
// 静态分配（不涉及任何动态内存）
static LED      led_status;
static RGBLight rgb_strip;

// 初始化（分配和初始化分离）
led_init(&led_status, 13, true);
rgb_light_init(&rgb_strip, 0x22);

// 风格1：Python 风格
led_status.fn.set_on(&led_status);

// 风格2：多态风格
Light *lights[] = {
    (Light *)&led_status,
    (Light *)&rgb_strip,
};
for (int i = 0; i < 2; i++)
    light_on(lights[i]);       // 各自调用自己的实现
```

---

## 10. 总结与进阶建议

### 10.1 你学到了什么

| 概念 | C 语言实现手法 | 对应代码 |
|---|---|---|
| **类（Class）** | `struct` 结构体 | `struct LED { ... }` |
| **对象（Object）** | 结构体变量 | `LED led;` |
| **方法（Method）** | 函数指针作为成员 | `led.fn.set_on(&led)` |
| **封装** | `.h` 暴露接口，`.c` 隐藏实现 | `led.h` vs `led.c` |
| **继承** | 基类作为派生类第一个成员 | `struct LED { Light base; ... }` |
| **多态** | 虚表（函数指针表） | `LightOps` + `ops` 指针 |
| **向上转型** | 把派生类指针转成基类指针 | `(Light *)&led` |

### 10.2 和 Python/C++ 的对应关系

```python
# Python
class Light:
    def on(self): pass   # 抽象方法

class LED(Light):        # 继承
    def on(self):        # 多态
        print("写GPIO")

led = LED()              # 实例化
led.on()                 # 对象.方法()
```

```c
// C （本 demo）
// 继承：struct LED { Light base; ... };
// 多态：static const LightOps led_ops = { .set_on = ... };
// 实例化：LED led; led_init(&led, ...);
// 方法调用：led.fn.set_on(&led);
// 多态调用：light_on((Light *)&led);
```

### 10.3 深入学习的建议

掌握了这些基础后，可以进一步学习：

1. **不透明指针（Opaque Pointer）模式**——完全隐藏结构体成员，只暴露类型名给用户
2. **对象池**——静态数组管理多个对象，避免碎片化
3. **回调函数 + 注册机制**——驱动层和应用层解耦
4. **状态机与 OOP 结合**——每个对象内部跑一个状态机
5. **C++ 对比学习**——学 C++ 的 class、virtual、继承语法，对比本 demo 的实现理解本质

### 10.4 常见问题

**Q: 为什么 C 的面向对象版本比面向过程版本代码多？**
A: 初期多了一些"框架"代码（虚表、方法表绑定），但换来的是**扩展新灯型时不需要修改已有代码**。项目越大，收益越明显。

**Q: 嵌入式 MCU 上跑得动吗？**
A: 完全跑得动。最终生成的机器码就是函数调用，不依赖任何运行时。函数指针表放在 Flash（常量区），不占宝贵的 RAM。

**Q: 一定要用函数指针吗？会不会降低性能？**
A: 函数指针调用比直接函数调用多一次间接寻址（先取指针再跳转），但编译器通常会内联优化。在嵌入式领域这点开销可以忽略不计。

**Q: 每个对象都存一份方法表，不会浪费内存吗？**
A: 本 demo 为了展示 `obj.fn.method()` 风格，让每个实例都有一份 `fn`。在 `main/` 目录下的"生产版本"中，方法只通过共享虚表调用，不占实例空间。两种模式可以根据需求选择。

**Q: 我可以直接在STM32上用这套代码吗？**
A: 把 `printf` 换成你的硬件操作（GPIO 写寄存器 / I2C 发数据），把 `main()` 改成对应的入口函数（如 `main` 或 `app_main`），就能直接用。

---

> **记住：OOP 不是语法糖，而是一种组织代码的思维方式。理解了本 demo 中的设计，你就理解了 C++ / Java / Python 中 class、extends、virtual、interface 这些关键字背后真正在做什么。**
