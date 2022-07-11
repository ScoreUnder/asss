#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include "search.h"

struct search_criteria {
    size_t *sames;
    size_t *diffs;
    size_t sames_len;
    size_t diffs_len;
};

struct search_criteria *generate_search_criteria_from_string(
    const char *string) {
    size_t string_len = strlen(string);

    struct search_criteria *restrict criteria =
        malloc(sizeof(struct search_criteria));
    criteria->sames = malloc(sizeof(size_t) * string_len * 2);
    criteria->sames_len = 0;
    criteria->diffs = malloc(string_len * sizeof(size_t));
    criteria->diffs_len = 0;

    uint8_t *checked = calloc(0x100, sizeof *checked);

    for (size_t i = 0; i < string_len; i++) {
        uint8_t c = string[i];
        if (checked[c] != 0) continue;
        checked[c] = 1;

        criteria->diffs[criteria->diffs_len++] = i;

        bool have_same = false;
        for (size_t j = i + 1; j < string_len; j++) {
            if (string[i] == string[j]) {
                if (!have_same) {
                    criteria->sames[criteria->sames_len++] = i;
                    have_same = true;
                }
                criteria->sames[criteria->sames_len++] = j;
            }
        }
        if (have_same) {
            criteria->sames[criteria->sames_len++] = SIZE_MAX;
        }
    }

    free(checked);

    criteria->sames =
        realloc(criteria->sames, criteria->sames_len * sizeof(size_t));
    criteria->diffs =
        realloc(criteria->diffs, criteria->diffs_len * sizeof(size_t));

    return criteria;
}

void free_search_criteria(struct search_criteria *criteria) {
    free(criteria->sames);
    free(criteria->diffs);
    free(criteria);
}

bool matches_criteria(const uint8_t *restrict data,
                      const struct search_criteria *restrict criteria) {
    size_t *sames = criteria->sames + criteria->sames_len;
    for (ptrdiff_t i = -criteria->sames_len; i + 1 < 0; i += 2) {
        uint8_t val = data[sames[i]];
        while (sames[i + 1] < SIZE_MAX) {
            if (val != data[sames[i + 1]]) return false;
            i++;
        }
    }

    size_t *diffs = criteria->diffs + criteria->diffs_len;
    for (ptrdiff_t i = -criteria->diffs_len; i + 1 < 0; i++) {
        uint8_t val = data[diffs[i]];
        for (ptrdiff_t j = i + 1; j < 0; j++) {
            if (val == data[diffs[j]]) return false;
        }
    }
    return true;
}

void print_debug(const struct search_criteria *restrict criteria,
                 const char *restrict search_string) {
    puts("Sames:");
    for (size_t i = 0; i < criteria->sames_len; i++) {
        size_t s = criteria->sames[i];
        if (s == SIZE_MAX) {
            printf("\n");
        } else {
            printf("%zu(%c) ", s, search_string[s]);
        }
    }
    puts("\nUniques:");
    for (size_t i = 0; i < criteria->diffs_len; i++) {
        size_t s = criteria->diffs[i];
        printf("%zu(%c) ", s, search_string[s]);
    }
    puts("\n");
}

#define MAX(x, y) ((x) > (y) ? (x) : (y))

void search_matches_in_file(FILE *input, const char *search_string, bool debug,
                            match_callback_fn match_callback, void *userdata) {
    size_t search_string_len = strlen(search_string);
    struct search_criteria *restrict criteria =
        generate_search_criteria_from_string(search_string);

    if (debug) {
        print_debug(criteria, search_string);
    }

    size_t buffer_capacity = MAX(0x1000, search_string_len * 2);
    uint8_t *buffer = malloc(buffer_capacity);
    size_t buffer_filled = 0;
    off_t abs_pos = 0;

    while (true) {
        size_t read = fread(&buffer[buffer_filled], 1,
                            buffer_capacity - buffer_filled, input);
        if (read == 0) break;

        buffer_filled += read;
        for (size_t i = 0; i <= buffer_filled - search_string_len; i++) {
            if (matches_criteria(&buffer[i], criteria)) {
                match_callback(i + abs_pos, &buffer[i], userdata);
            }
        }

        size_t wrap_amt = search_string_len - 1;
        memmove(buffer, &buffer[buffer_filled - wrap_amt], wrap_amt);
        abs_pos += buffer_filled - wrap_amt;
        buffer_filled = wrap_amt;
    }

    free(buffer);
    free_search_criteria(criteria);
}
