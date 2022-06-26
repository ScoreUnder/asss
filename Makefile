TOOLCHAIN_PREFIX =
CC = $(TOOLCHAIN_PREFIX)gcc

PROFILE = debug

GTK_FLAGS != $(TOOLCHAIN_PREFIX)pkg-config --cflags gtk4
GTK_LIBS  != $(TOOLCHAIN_PREFIX)pkg-config --libs   gtk4

CPPFLAGS = -Isrc -D_POSIX_C_SOURCE=200809L
CFLAGS_PR_debug = -Og -ggdb -Werror
CFLAGS_PR_release = -O3 -ggdb
CFLAGS   = $(GTK_FLAGS) $(CFLAGS_PR_$(PROFILE)) -Wall -Wextra -pedantic -std=c11 $(CUSTOM_CFLAGS)

OBJS = src/main.o src/search.o src/display.o
OBJS_GUI = src/gui/main_gtk.o src/gui/search_results_model.o src/search.o src/display.o

OUT = asss
OUT_GUI = asss-gui

all: $(OUT) $(OUT_GUI)

$(OUT): $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

$(OUT_GUI): $(OBJS_GUI)
	$(LINK.c) -o $@ $(OBJS_GUI) $(GTK_LIBS)

clean:
	-rm -f -- $(OBJS) $(OUT) $(OBJS_GUI) $(OUT_GUI) $(OBJS:.o=.gcda)

.SUFFIXES: .c .o

.c.o:
	$(COMPILE.c) -o $@ $<

.PHONY: all clean
