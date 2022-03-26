#ifndef ASSS_DISPLAY_H
#define ASSS_DISPLAY_H 1

#include <stdint.h>
#include <stddef.h>

void make_decode_table(char tl_table[0x100], const uint8_t *from,
                       const uint8_t *to, size_t len);

uint16_t find_base(uint16_t base, uint8_t char_index, size_t i);

void make_speculative(char speculative[0x100], const char table[0x100]);

#endif