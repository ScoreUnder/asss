#ifndef ASSS_DISPLAY_H
#define ASSS_DISPLAY_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    char *exact;
    char *guess;
    char *unknown;
    char *end;
    char *changeover;
    bool needs_html_escape;
} match_colours;

#ifdef __TINYC__  // TCC does not support "static" in array parameters
#define STATIC
#else
#define STATIC static
#endif

void make_decode_table(char tl_table[STATIC 0x100], const uint8_t *from,
                       const uint8_t *to, size_t len);

uint16_t find_base(uint16_t base, uint8_t char_index, size_t i);

void make_speculative(char speculative[STATIC 0x100],
                      const char table[STATIC 0x100]);

typedef void printflike(void *userdata, const char *format, ...);

void print_detailed_result(FILE *input, off_t offset, const char *search_str,
                           size_t search_str_len, const match_colours *colours,
                           printflike *pf, void *userdata);

#endif
