#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "search.h"

void print_translation_of(const uint8_t *from, const uint8_t *to, size_t len) {
    char tl_table[0x100];
    memset(tl_table, 0, 0x100);

    for (size_t i = 0; i < len; i++) {
        if (tl_table[from[i]] != 0 && tl_table[from[i]] != to[i]) {
            printf("conflict at pos %zi\n", i);
        }
        tl_table[from[i]] = to[i];
    }

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

struct search_str {
    const char *str;
    size_t len;
};

void match_callback(off_t pos, const uint8_t *buffer, void *userdata) {
    struct search_str *str = (struct search_str *)userdata;
    printf("%llx: ", (unsigned long long) pos);
    print_translation_of(buffer, (const uint8_t*) str->str, str->len);
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

    struct search_str str = {
        .str = argv[2],
        .len = strlen(argv[2])
    };

    search_matches_in_file(input, str.str, true, match_callback, &str);

    fclose(input);
    return 0;
}
