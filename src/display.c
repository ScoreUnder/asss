#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
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
