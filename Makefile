CFLAGS = -O2 -ggdb -Wall -Wextra -pedantic -std=c11
OBJS = src/main.o src/search.o
OUT = asss

$(OUT): $(OBJS)
	$(LINK.c) -o $@ $^

clean:
	-rm -f -- $(OBJS) $(OUT)

.PHONY: clean
