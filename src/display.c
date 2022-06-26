#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "display.h"

void make_decode_table(char tl_table[0x100], const uint8_t *from,
                       const uint8_t *to, size_t len) {
    memset(tl_table, 0, 0x100);

    for (size_t i = 0; i < len; i++) {
        assert(tl_table[from[i]] == 0 || tl_table[from[i]] == to[i]);
        tl_table[from[i]] = to[i];
    }
}

const uint16_t missing = 0x100;
const uint16_t conflict = 0x101;

uint16_t find_base(uint16_t base, uint8_t char_index, size_t i) {
    uint16_t new_base = i - char_index;
    if (base == missing && char_index <= i) {
        return new_base;
    } else if (base != new_base) {
        return conflict;
    } else {
        return base;
    }
}

void make_speculative(char speculative[0x100], const char table[0x100]) {
    memset(speculative, 0, 0x100);

    uint16_t upper_base = missing;
    uint16_t lower_base = missing;
    uint16_t digit_base = missing;

    for (size_t i = 0; i < 0x100; i++) {
        if (table[i] >= 'A' && table[i] <= 'Z') {
            upper_base = find_base(upper_base, table[i] - 'A', i);
        } else if (table[i] >= 'a' && table[i] <= 'z') {
            lower_base = find_base(lower_base, table[i] - 'a', i);
        } else if (table[i] >= '0' && table[i] <= '9') {
            digit_base = find_base(digit_base, table[i] - '0', i);
        }
    }

    if (upper_base < 0x100) {
        for (size_t i = 0; i < 26 && i + upper_base < 0x100; i++) {
            speculative[i + upper_base] = 'A' + i;
        }
    }
    if (lower_base < 0x100) {
        for (size_t i = 0; i < 26 && i + lower_base < 0x100; i++) {
            speculative[i + lower_base] = 'a' + i;
        }
    }
    if (digit_base < 0x100) {
        for (size_t i = 0; i < 10 && i + digit_base < 0x100; i++) {
            speculative[i + digit_base] = '0' + i;
        }
    }
}

void print_hex_result(char table[0x100], char speculative[0x100],
                      const uint8_t *buf, size_t len,
                      const match_colours *colours, printflike *pf,
                      void *userdata) {
    const char *last_colour = NULL;
    for (size_t i = 0; i < len; i++) {
        uint8_t c = buf[i];
        const char *colour;
        if (table[c])
            colour = colours->exact;
        else if (speculative[c])
            colour = colours->guess;
        else
            colour = colours->unknown;

        if (colour != last_colour) {
            const char *changeover =
                last_colour == NULL ? "" : colours->changeover;
            pf(userdata, "%s%s", changeover, colour);
            last_colour = colour;
        }
        pf(userdata, "%02hhx", buf[i]);
    }
    pf(userdata, "%s", colours->end);
}

void print_text_result(char table[0x100], char speculative[0x100],
                       const uint8_t *buf, size_t len,
                       const match_colours *colours, printflike *pf,
                       void *userdata) {
    const char *last_colour = NULL;
    for (size_t i = 0; i < len; i++) {
        uint8_t c = buf[i];
        char decoded = ' ';
        const char *colour;
        if (table[c]) {
            colour = colours->exact;
            decoded = table[c];
        } else if (speculative[c]) {
            colour = colours->guess;
            decoded = speculative[c];
        } else {
            colour = colours->unknown;
        }

        if (colour != last_colour) {
            const char *changeover =
                last_colour == NULL ? "" : colours->changeover;
            pf(userdata, "%s%s", changeover, colour);
            last_colour = colour;
        }

        if (colours->needs_html_escape && decoded == '<') {
            pf(userdata, " &lt;");
        } else {
            pf(userdata, "%2c", decoded);
        }
    }
    pf(userdata, "%s", colours->end);
}

void print_detailed_result(FILE *input, off_t offset, const char *search_str,
                           size_t search_str_len, const match_colours *colours,
                           printflike *pf, void *userdata) {
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
            // TODO: sensible GUI display for these errors
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

    pf(userdata, "%#010llx ", (unsigned long long)(offset - pre_len));
    size_t pad_len = search_str_len - pre_len;
    pf(userdata, "%*s", (int)pad_len * 2, "");
    print_hex_result(table, speculative, buf, pre_len, colours, pf, userdata);
    pf(userdata, "\n%*s", (int)pad_len * 2 + 11, "");
    print_text_result(table, speculative, buf, pre_len, colours, pf, userdata);

    size_t remaining = read_len - pre_len;
    size_t print_offset = pre_len;
    while (remaining > 0) {
        size_t print_len = remaining;
        if (print_len > search_str_len) {
            print_len = search_str_len;
        }
        pf(userdata, "\n%#010llx ",
           (unsigned long long)(offset + print_offset - pre_len));
        print_hex_result(table, speculative, buf + print_offset, print_len,
                         colours, pf, userdata);
        pf(userdata, "\n%*s", 11, "");
        print_text_result(table, speculative, buf + print_offset, print_len,
                          colours, pf, userdata);
        remaining -= print_len;
        print_offset += print_len;
    }
    pf(userdata, "\n\n");

cleanup:
    free(buf);
}
