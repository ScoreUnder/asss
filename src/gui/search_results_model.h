#ifndef SEARCH_RESULTS_MODEL_H
#define SEARCH_RESULTS_MODEL_H 1

#include <gtk/gtk.h>
#include <stdio.h>

G_BEGIN_DECLS

#define ASSS_TYPE_SEARCH_RESULT asss_search_result_get_type()
G_DECLARE_FINAL_TYPE(AsssSearchResult, asss_search_result, ASSS, SEARCH_RESULT,
                     GObject)

AsssSearchResult *asss_search_result_new(off_t address, gchararray text);

GtkListItemFactory *asss_make_address_column_factory(void);
GtkListItemFactory *asss_make_result_column_factory(void);

#endif
