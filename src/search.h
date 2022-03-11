#ifndef ASSS_SEARCH_H
#define ASSS_SEARCH_H 1

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

struct search_criteria;

struct search_criteria *
generate_search_criteria_from_string(const char *string);

void free_search_criteria(struct search_criteria *criteria);

bool matches_criteria(const uint8_t *data,
                      const struct search_criteria *criteria);

void print_debug(const struct search_criteria *criteria,
                 const char *search_string);

typedef void (*match_callback_fn)(off_t pos, const uint8_t *buffer,
                                  void *userdata);

void search_matches_in_file(FILE *input, const char *search_string, bool debug,
                            match_callback_fn match_callback, void *userdata);

#endif
