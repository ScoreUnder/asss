TOOLCHAIN_PREFIX =
CC = $(TOOLCHAIN_PREFIX)gcc

PROFILE = debug

GTK_FLAGS != $(TOOLCHAIN_PREFIX)pkg-config --cflags gtk4
GTK_LIBS  != $(TOOLCHAIN_PREFIX)pkg-config --libs   gtk4

CPPFLAGS = -Isrc
CFLAGS_PR_debug = -Og -ggdb -Werror
CFLAGS_PR_release = -O3 -ggdb
CFLAGS   = $(GTK_FLAGS) $(CFLAGS_PR_$(PROFILE)) -Wall -Wextra -pedantic -std=c11 $(CUSTOM_CFLAGS)

OBJS = src/main.o src/search.o
OBJS_GUI = src/gui/main_gtk.o src/gui/search_results_model.o src/search.o

OUT = asss
OUT_GUI = asss-gui

all: $(OUT) $(OUT_GUI)

$(OUT): $(OBJS)
	$(LINK.c) -o $@ $^

$(OUT_GUI): $(OBJS_GUI)
	$(LINK.c) -o $@ $^ $(GTK_LIBS)

clean:
	-rm -f -- $(OBJS) $(OUT) $(OBJS_GUI) $(OUT_GUI)

.PHONY: all clean
