#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "display.h"

void make_decode_table(char tl_table[STATIC 0x100], const uint8_t *from,
                       const uint8_t *to, size_t len) {
    memset(tl_table, 0, 0x100);

    for (size_t i = 0; i < len; i++) {
        assert(tl_table[from[i]] == 0 || tl_table[from[i]] == to[i]);
        tl_table[from[i]] = to[i];
    }
}

static const uint16_t missing = 0x100;
static const uint16_t conflict = 0x101;

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

static void fill_speculative_sequence(char speculative[STATIC 0x100], int base,
                                      char base_ascii, size_t base_size) {
    for (size_t i = 0; i < base_size && i + base < 0x100; i++) {
        speculative[i + base] = base_ascii + i;
    }
}

void make_speculative(char speculative[STATIC 0x100],
                      const char table[STATIC 0x100]) {
    memset(speculative, 0, 0x100);

    uint16_t upper_base = missing;
    uint16_t lower_base = missing;
    uint16_t digit_base = missing;

    for (size_t i = 0; i < 0x100; i++) {
        // Note: this program uses the C locale; the following is<x> functions
        // must all assume ASCII for this procedure to make sense.
        if (isupper(table[i])) {
            upper_base = find_base(upper_base, table[i] - 'A', i);
        } else if (islower(table[i])) {
            lower_base = find_base(lower_base, table[i] - 'a', i);
        } else if (isdigit(table[i])) {
            digit_base = find_base(digit_base, table[i] - '0', i);
        }
    }

    fill_speculative_sequence(speculative, upper_base, 'A', 26);
    fill_speculative_sequence(speculative, lower_base, 'a', 26);
    fill_speculative_sequence(speculative, digit_base, '0', 10);
}

static const char hex_chars[] = "0123456789abcdef";

void print_hex_result(char table[STATIC 0x100], char speculative[STATIC 0x100],
                      const uint8_t *buf, size_t len,
                      const match_colours *colours, putslike *put,
                      void *userdata) {
    const char *last_colour = NULL;
    char hexbuf[3];
    hexbuf[2] = '\0';
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
            if (last_colour != NULL)
                put(userdata, colours->changeover);
            put(userdata, colour);
            last_colour = colour;
        }
        hexbuf[0] = hex_chars[buf[i] >> 4];
        hexbuf[1] = hex_chars[buf[i] & 0xf];
        put(userdata, hexbuf);
    }
    if (last_colour != NULL)
        put(userdata, colours->end);
}

void print_text_result(char table[STATIC 0x100], char speculative[STATIC 0x100],
                       const uint8_t *buf, size_t len,
                       const match_colours *colours, putslike *put,
                       void *userdata) {
    const char *last_colour = NULL;
    char formatbuf[3];
    formatbuf[0] = ' ';
    formatbuf[2] = '\0';
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
            if (last_colour != NULL)
                put(userdata, colours->changeover);
            put(userdata, colour);
            last_colour = colour;
        }

        if (colours->needs_html_escape && decoded == '<') {
            put(userdata, " &lt;");
        } else {
            formatbuf[1] = decoded;
            put(userdata, formatbuf);
        }
    }
    if (last_colour != NULL)
        put(userdata, colours->end);
}

void print_detailed_result(FILE *input, off_t offset, const char *search_str,
                           size_t search_str_len, const match_colours *colours,
                           putslike *put, void *userdata) {
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

    size_t pad_len = search_str_len - pre_len;

    char *formatbuf = malloc(pad_len * 2 + 13);

    sprintf(formatbuf, "%#010llx %*s", (unsigned long long)(offset - pre_len), (int)pad_len * 2, "");
    put(userdata, formatbuf);

    print_hex_result(table, speculative, buf, pre_len, colours, put, userdata);

    sprintf(formatbuf, "\n%*s", (int)pad_len * 2 + 11, "");
    put(userdata, formatbuf);

    print_text_result(table, speculative, buf, pre_len, colours, put, userdata);

    size_t remaining = read_len - pre_len;
    size_t print_offset = pre_len;
    while (remaining > 0) {
        size_t print_len = remaining;
        if (print_len > search_str_len) {
            print_len = search_str_len;
        }

        sprintf(formatbuf, "\n%#010llx ", (unsigned long long)(offset + print_offset - pre_len));
        put(userdata, formatbuf);

        print_hex_result(table, speculative, buf + print_offset, print_len,
                         colours, put, userdata);

        sprintf(formatbuf, "\n%*s", 11, "");
        put(userdata, formatbuf);

        print_text_result(table, speculative, buf + print_offset, print_len,
                          colours, put, userdata);
        remaining -= print_len;
        print_offset += print_len;
    }
    put(userdata, "\n\n");

    free(formatbuf);
cleanup:
    free(buf);
}
