GTK_FLAGS != pkg-config --cflags gtk4
GTK_LIBS  != pkg-config --libs   gtk4

CPPFLAGS = -Isrc
CFLAGS   = $(GTK_FLAGS) -Og -ggdb -Wall -Wextra -pedantic -std=c11

OBJS = src/main.o src/search.o
OBJS_GUI = src/gui/main_gtk.o src/gui/search_results_model.o src/search.o

OUT = asss
OUT_GUI = asss-gui

$(OUT): $(OBJS)
	$(LINK.c) -o $@ $^

$(OUT_GUI): $(OBJS_GUI)
	$(LINK.c) $(GTK_LIBS) -o $@ $^

clean:
	-rm -f -- $(OBJS) $(OUT) $(OBJS_GUI) $(OUT_GUI)

.PHONY: clean
