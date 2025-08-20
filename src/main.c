#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "search.h"
#include "display.h"

match_colours term_colours = {
    .end = "\033[0m",
    .changeover = "",
    .exact = "\033[94m",
    .guess = "\033[93m",
    .unknown = "\033[91m",
    .needs_html_escape = false,
};

void print_translation_of(const uint8_t *from, const uint8_t *to, size_t len) {
    uint8_t tl_table[0x100];
    make_decode_table(tl_table, from, to, len);

    for (size_t i = 0; i < 0x100; i++) {
        if (tl_table[i] != 0) {
            if (tl_table[i] == '\'' || tl_table[i] == '\\') {
                printf("[0x%02zX] = '\\%c', ", i, tl_table[i]);
            } else {
                printf("[0x%02zX] = '%c', ", i, tl_table[i]);
            }
        }
    }
}

void put_puts(void *_unused, const char *str) {
    (void)_unused;
    fputs(str, stdout);
}

struct search_state {
    const char *str;
    size_t len;
    off_t *offsets;
    size_t offset_count;
    size_t offset_capacity;
};

void add_offset(struct search_state *state, off_t offset) {
    if (state->offset_count == state->offset_capacity) {
        state->offset_capacity =
            (state->offset_capacity == 0) ? 16 : state->offset_capacity * 2;
        state->offsets = realloc(
            state->offsets, state->offset_capacity * sizeof state->offsets[0]);
    }
    state->offsets[state->offset_count++] = offset;
}

void match_callback(off_t pos, const uint8_t *buffer, void *userdata) {
    struct search_state *state = userdata;
    printf("%llx: ", (unsigned long long)pos);
    print_translation_of(buffer, (const uint8_t *)state->str, state->len);
    add_offset(state, pos);
    putc('\n', stdout);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <input> <search_string>\n", argv[0]);
        return 1;
    }

    FILE *input = fopen(argv[1], "rb");
    if (!input) {
        perror(argv[1]);
        return 1;
    }

    struct search_state state = {
        .str = argv[2],
        .len = strlen(argv[2]),
        .offsets = NULL,
        .offset_count = 0,
        .offset_capacity = 0,
    };

    search_matches_in_file(input, state.str, true, match_callback, &state);

    printf("\n%zu matches\n", state.offset_count);
    if (state.offset_count > 0) {
        puts("Detailed results:\n");

        for (size_t i = 0; i < state.offset_count; i++) {
            print_detailed_result(input, state.offsets[i], state.str, state.len,
                                  &term_colours, put_puts, NULL);
        }
    }

    free(state.offsets);
    fclose(input);
    return 0;
}
