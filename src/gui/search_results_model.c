#include "search_results_model.h"
#include <gtk/gtk.h>
#include <stdio.h>

struct _AsssSearchResult {
  GObject parent;
  off_t address;
  gchararray text; // temporary until this is dynamically generated
};

G_DEFINE_TYPE(AsssSearchResult, asss_search_result, G_TYPE_OBJECT)

AsssSearchResult *asss_search_result_new(off_t address, gchararray text) {
  AsssSearchResult *result = g_object_new(ASSS_TYPE_SEARCH_RESULT, NULL);
  result->address = address;
  result->text = g_strdup(text);
  return result;
}

static void asss_search_result_init(AsssSearchResult *self) {
  // Should not be called without new()
  (void)self;
}

static void asss_search_result_class_init(AsssSearchResultClass *klass) {
  (void)klass;
}

static void text_column_setup(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer user_data) {
  (void)factory;
  (void)user_data;
  GtkWidget *label = gtk_label_new(NULL);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_list_item_set_child(item, label);
}

static void text_column_unbind(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer user_data) {
  (void)factory;
  (void)user_data;
  GtkWidget *label = gtk_list_item_get_child(item);
  gtk_label_set_text(GTK_LABEL(label), "");
}

static void text_column_teardown(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer user_data) {
  (void)factory;
  (void)user_data;
  gtk_list_item_set_child(item, NULL);
}

static void address_column_bind(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer user_data) {
  (void)factory;
  (void)user_data;
  GtkWidget *label = gtk_list_item_get_child(item);
  AsssSearchResult *result = (AsssSearchResult *)gtk_list_item_get_item(item);
  gchararray text = g_strdup_printf("%08llx", (unsigned long long) result->address);
  gtk_label_set_text(GTK_LABEL(label), text);
  g_free(text);
}

static void result_column_bind(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer user_data) {
  (void)factory;
  (void)user_data;
  GtkWidget *label = gtk_list_item_get_child(item);
  AsssSearchResult *result = (AsssSearchResult *)gtk_list_item_get_item(item);
  gtk_label_set_text(GTK_LABEL(label), result->text);
}

GtkListItemFactory *asss_make_address_column_factory(void) {
  GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(text_column_setup), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(address_column_bind), NULL);
  g_signal_connect(factory, "unbind", G_CALLBACK(text_column_unbind), NULL);
  g_signal_connect(factory, "teardown", G_CALLBACK(text_column_teardown), NULL);
  return factory;
}

GtkListItemFactory *asss_make_result_column_factory(void) {
  GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(text_column_setup), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(result_column_bind), NULL);
  g_signal_connect(factory, "unbind", G_CALLBACK(text_column_unbind), NULL);
  g_signal_connect(factory, "teardown", G_CALLBACK(text_column_teardown), NULL);
  return factory;
}
