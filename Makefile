TOOLCHAIN_PREFIX =
CC = $(TOOLCHAIN_PREFIX)gcc

GTK_FLAGS != $(TOOLCHAIN_PREFIX)pkg-config --cflags gtk4
GTK_LIBS  != $(TOOLCHAIN_PREFIX)pkg-config --libs   gtk4

CPPFLAGS = -Isrc
CFLAGS   = $(GTK_FLAGS) -Og -ggdb -Wall -Wextra -pedantic -std=c11

OBJS = src/main.o src/search.o
OBJS_GUI = src/gui/main_gtk.o src/gui/search_results_model.o src/search.o

OUT = asss
OUT_GUI = asss-gui

$(OUT): $(OBJS)
	$(LINK.c) -o $@ $^

$(OUT_GUI): $(OBJS_GUI)
	$(LINK.c) -o $@ $^ $(GTK_LIBS)

clean:
	-rm -f -- $(OBJS) $(OUT) $(OBJS_GUI) $(OUT_GUI)

.PHONY: clean
