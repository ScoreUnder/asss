CFLAGS = -O2 -ggdb
OBJS = src/main.o
OUT = asss

$(OUT): src/main.o
	$(LINK.c) -o $@ $^

clean:
	-rm -f -- $(OBJS) $(OUT)

.PHONY: clean
