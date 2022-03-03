#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

struct search_criteria {
    size_t *sames;
    size_t *diffs;
    size_t *same_gaps;
    size_t sames_len;
    size_t diffs_len;
    size_t same_gaps_len;
};

struct search_criteria *generate_search_criteria_from_string(const char *string) {
    size_t string_len = strlen(string);

    struct search_criteria *restrict criteria = malloc(sizeof(struct search_criteria));
    criteria->sames = malloc(sizeof(size_t) * string_len * 2);
    criteria->sames_len = 0;
    criteria->diffs = malloc(string_len * sizeof(size_t));
    criteria->diffs_len = 0;

    uint8_t *checked = malloc(0x100);
    memset(checked, 0, 0x100);

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

    criteria->sames = realloc(criteria->sames, criteria->sames_len * sizeof(size_t));
    criteria->diffs = realloc(criteria->diffs, criteria->diffs_len * sizeof(size_t));

    criteria->same_gaps = malloc(sizeof(size_t *) * string_len);
    memset(criteria->same_gaps, 0xFF, sizeof(size_t *) * string_len);
    criteria->same_gaps_len = 0;

    for (size_t i = 0; i + 1 < criteria->sames_len; i++) {
        size_t start = criteria->sames[i];
        size_t end = criteria->sames[i + 1];
        if (end == SIZE_MAX) {
            i++;
            continue;
        }

        size_t gap_size = end - start - 1;

        if (criteria->same_gaps[gap_size] > start)
            criteria->same_gaps[gap_size] = start;

        if (criteria->same_gaps_len <= gap_size)
            criteria->same_gaps_len = gap_size + 1;
    }

    criteria->same_gaps = realloc(criteria->same_gaps, sizeof(size_t) * criteria->same_gaps_len);

    return criteria;
}

size_t get_skip_from_same_gap(const struct search_criteria *restrict criteria, size_t start, size_t end, size_t search_len) {
    size_t gap_size = end - start - 1;
    if (gap_size < criteria->same_gaps_len) {
        size_t first_gap_pos = criteria->same_gaps[gap_size];

        assert(first_gap_pos != start);
        if (start > first_gap_pos) {
            return start - first_gap_pos;
        }
    }

    // No possible same-gaps to skip to, skip until this gap doesn't exist
    return start + 1;
}

size_t get_search_skip(const uint8_t *restrict data, const struct search_criteria *restrict criteria, size_t search_len) {
    for (size_t i = 0; i < criteria->diffs_len; i++) {
        uint8_t val = data[criteria->diffs[i]];
        for (size_t j = i + 1; j < criteria->diffs_len; j++) {
            if (val == data[criteria->diffs[j]])
                return get_skip_from_same_gap(criteria, criteria->diffs[i], criteria->diffs[j], search_len);
        }
    }

    for (size_t i = 0; i + 1 < criteria->sames_len; i++) {
        uint8_t val = data[criteria->sames[i]];
        while (criteria->sames[i + 1] != SIZE_MAX) {
            if (val != data[criteria->sames[i + 1]])
                return 1;
            i++;
        }
        i++;
    }
    return 0;
}

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

#define MAX(x, y) ((x) > (y) ? (x) : (y))

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

    char *search_string = argv[2];
    size_t search_string_len = strlen(search_string);
    struct search_criteria *restrict criteria = generate_search_criteria_from_string(search_string);

    // debug: print search criteria
    puts("Sames:");
    for (size_t i = 0; i < criteria->sames_len; i++) {
        size_t s = criteria->sames[i];
        if (s == SIZE_MAX) {
            printf("\n");
        } else {
            printf("%zu(%c) ", s, search_string[s]);
        }
    }
    puts("\nGaps:");
    for (size_t i = 0; i < criteria->same_gaps_len; i++) {
        if (criteria->same_gaps[i] != SIZE_MAX) printf("%zu @ %zu\n", i, criteria->same_gaps[i]);
    }
    puts("\nUniques:");
    for (size_t i = 0; i < criteria->diffs_len; i++) {
        size_t s = criteria->diffs[i];
        printf("%zu(%c) ", s, search_string[s]);
    }
    puts("\n");

    size_t buffer_capacity = MAX(0x1000, search_string_len * 2);
    uint8_t *buffer = malloc(buffer_capacity);
    size_t buffer_filled = 0;
    size_t abs_pos = 0;

    while (true) {
        size_t read = fread(&buffer[buffer_filled], 1, buffer_capacity - buffer_filled, input);
        if (read == 0)
            break;

        buffer_filled += read;
        for (size_t i = 0; i <= buffer_filled - search_string_len; i++) {
            size_t skip = get_search_skip(&buffer[i], criteria, search_string_len);
            if (skip == 0) {
                printf("%zx: ", i + abs_pos);
                print_translation_of(&buffer[i], (const uint8_t*) search_string, search_string_len);
                putc('\n', stdout);
            } else {
                i += skip - 1;
            }
        }

        size_t wrap_amt = search_string_len - 1;
        memmove(buffer, &buffer[buffer_filled - wrap_amt], wrap_amt);
        abs_pos += buffer_filled - wrap_amt;
        buffer_filled = wrap_amt;
    }

    fclose(input);
    free(buffer);
    free(criteria->sames);
    free(criteria->diffs);
    free(criteria->same_gaps);
    free(criteria);
    return 0;
}
