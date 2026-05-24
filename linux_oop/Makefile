# Makefile — Linux内核风格OOP demo 构建文件
#
# 用法：
#   make         编译 demo
#   make run     编译并运行
#   make clean   清理编译产物
#   make doc     在浏览器中查看文档（仅 Linux/macOS）

CC      := gcc
CFLAGS  := -Wall -Wextra -Werror -Wstrict-prototypes \
           -Wmissing-prototypes -Wpointer-arith \
           -std=gnu99 \
           -I include -I devices \
           -g -O0
LDFLAGS :=
RM      := rm -f

# 源文件
SRCS := \
	devices/device.c \
	devices/gpio_led.c \
	devices/i2c_rgb.c \
	devices/pwm_buzzer.c \
	demo/main.c

OBJS := $(SRCS:.c=.o)
TARGET := demo

.PHONY: all run clean doc

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(OBJS) $(TARGET)

doc:
	@echo "Open DOC.md in your editor or browser."
	@echo "On Linux/macOS:"
	@echo "  xdg-open DOC.md  # Linux"
	@echo "  open DOC.md      # macOS"
	@echo "On Windows: start DOC.md"
