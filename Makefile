CFLAGS = -O2 -ggdb -Wall -Wextra -pedantic -std=c11
OBJS = src/main.o
OUT = asss

$(OUT): src/main.o
	$(LINK.c) -o $@ $^

%.s: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S -masm=intel -o $@ $<

clean:
	-rm -f -- $(OBJS) $(OUT)

.PHONY: clean
