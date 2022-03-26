#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "search.h"
#include "display.h"

void print_translation_of(const uint8_t *from, const uint8_t *to, size_t len) {
    char tl_table[0x100];
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

void print_hex_result(char table[0x100], char speculative[0x100],
                      const uint8_t *buf, size_t len) {
    int last_colour = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t c = buf[i];
        int colour;
        if (table[c])
            colour = 4;
        else if (speculative[c])
            colour = 3;
        else
            colour = 1;

        if (colour != last_colour) {
            printf("\033[9%dm", colour);
            last_colour = colour;
        }
        printf("%02hhx", buf[i]);
    }
    printf("\033[0m");
}

void print_text_result(char table[0x100], char speculative[0x100],
                       const uint8_t *buf, size_t len) {
    int last_colour = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t c = buf[i];
        char decoded = ' ';
        int colour;
        if (table[c]) {
            colour = 4;
            decoded = table[c];
        } else if (speculative[c]) {
            colour = 3;
            decoded = speculative[c];
        } else {
            colour = 1;
        }

        if (colour != last_colour) {
            printf("\033[9%dm", colour);
            last_colour = colour;
        }
        printf("%2c", decoded);
    }
    printf("\033[0m");
}

void print_detailed_result(FILE *input, off_t offset, const char *search_str,
                           size_t search_str_len) {
    size_t pre_len = search_str_len;
    if (offset < (off_t)search_str_len) {
        pre_len = (size_t)offset;
    }
    size_t post_len = search_str_len;

    fseek(input, offset - (off_t)pre_len, SEEK_SET);
    uint8_t *buf = malloc(search_str_len + pre_len + post_len);
    size_t read_len = fread(buf, 1, search_str_len + pre_len + post_len, input);
    if (read_len < search_str_len + pre_len + post_len) {
        if (read_len >= search_str_len + pre_len) {
            post_len = read_len - (search_str_len + pre_len);
        } else {
            // Read error; should always have enough bytes in the source file to
            // recall the original match.
            fprintf(stderr, "Read error when asking for %zu bytes at %#llx.\n",
                    search_str_len + pre_len + post_len,
                    (unsigned long long)offset);
            fprintf(stderr, "Needed at least %zu bytes, but only got %zu.\n",
                    search_str_len + pre_len, read_len);
            fprintf(stderr,
                    "This should not happen, because a match was already found "
                    "here in an earlier pass. This is indicative of a "
                    "concurrently-modified file or a bug in this program.\n");
            goto cleanup;
        }
    }

    char table[0x100];
    make_decode_table(table, buf + pre_len, (uint8_t *)search_str,
                      search_str_len);

    char speculative[0x100];
    make_speculative(speculative, table);

    printf("%#010llx ", (unsigned long long)(offset - pre_len));
    size_t pad_len = search_str_len - pre_len;
    printf("%*s", (int)pad_len * 2, "");
    print_hex_result(table, speculative, buf, pre_len);
    printf("\n%*s", (int)pad_len * 2 + 11, "");
    print_text_result(table, speculative, buf, pre_len);

    size_t remaining = read_len - pre_len;
    size_t print_offset = pre_len;
    while (remaining > 0) {
        size_t print_len = remaining;
        if (print_len > search_str_len) {
            print_len = search_str_len;
        }
        printf("\n%#010llx ",
               (unsigned long long)(offset + print_offset - pre_len));
        print_hex_result(table, speculative, buf + print_offset, print_len);
        printf("\n%*s", 11, "");
        print_text_result(table, speculative, buf + print_offset, print_len);
        remaining -= print_len;
        print_offset += print_len;
    }
    printf("\n\n");

cleanup:
    free(buf);
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
            print_detailed_result(input, state.offsets[i], state.str,
                                  state.len);
        }
    }

    free(state.offsets);
    fclose(input);
    return 0;
}
