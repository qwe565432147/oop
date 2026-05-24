CC       = gcc
CFLAGS   = -Wall -Wextra -std=c99 -pedantic
TARGET   = oop_demo
SRCS     = main.c led.c rgb_light.c
OBJS     = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
