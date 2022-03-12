#include "search.h"
#include "search_results_model.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>

struct browse_clicked_data {
  GtkWindow *window;
  GtkEntry *entry;
  GtkWidget *next;
};

void browse_dialog_response(GtkDialog *dialog, gint response_id,
                            gpointer user_data) {
  struct browse_clicked_data *data = user_data;
  if (response_id == GTK_RESPONSE_ACCEPT) {
    GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
    if (file) {
      gchar *path = g_file_get_path(file);
      GtkEntryBuffer *buffer = gtk_entry_get_buffer(data->entry);
      gtk_entry_buffer_set_text(buffer, path, -1);
      g_free(path);
    }
  }
  gtk_window_destroy(GTK_WINDOW(dialog));
  gtk_window_set_focus(data->window, data->next);
}

void browse_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  struct browse_clicked_data *data = user_data;
  GtkWidget *dialog = gtk_file_chooser_dialog_new(
      "Open ROM", data->window, GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel",
      GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
  g_signal_connect(dialog, "response", G_CALLBACK(browse_dialog_response),
                   data);
  gtk_widget_show(dialog);
}

void free_from_closure(gpointer data, GClosure *closure) {
  (void)closure;
  g_free(data);
}

struct search_clicked_data {
  GtkEditable *file_entry;
  GtkEditable *search_entry;
  GListStore *results_store;
};

struct search_find_data {
  GArray *results;
  const char *search_text;
  size_t search_text_len;
};

void search_result_callback(off_t pos, const uint8_t *buffer, void *user_data) {
  struct search_find_data *find_data = user_data;
  GString *str = g_string_new("");

  char tl_table[0x100];
  memset(tl_table, 0, 0x100);

  for (size_t i = 0; i < find_data->search_text_len; i++) {
    tl_table[buffer[i]] = find_data->search_text[i];
  }

  for (size_t i = 0; i < 0x100; i++) {
    if (tl_table[i] != 0) {
      if (tl_table[i] == '\'' || tl_table[i] == '\\') {
        g_string_append_printf(str, "[0x%02zX] = '\\%c', ", i, tl_table[i]);
      } else {
        g_string_append_printf(str, "[0x%02zX] = '%c', ", i, tl_table[i]);
      }
    }
  }

  AsssSearchResult *result = asss_search_result_new(pos, str->str);
  g_array_append_vals(find_data->results, &result, 1);

  g_string_free(str, TRUE);
}

void search_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  struct search_clicked_data *data = user_data;
  g_list_store_remove_all(data->results_store);

  char *file_path = gtk_editable_get_chars(data->file_entry, 0, -1);
  char *search_text = gtk_editable_get_chars(data->search_entry, 0, -1);

  if (file_path && search_text && strlen(search_text) >= 2) {
    FILE *input = NULL;
    input = fopen(file_path, "rb");
    if (input) {
      struct search_find_data find_data = {
          .results = g_array_new(FALSE, FALSE, sizeof(gpointer *)),
          .search_text = search_text,
          .search_text_len = strlen(search_text),
      };
      search_matches_in_file(input, search_text, false, search_result_callback,
                             &find_data);
      fclose(input);
      g_list_store_splice(data->results_store, 0, 0,
                          (gpointer *)find_data.results->data,
                          find_data.results->len);
      g_array_free(find_data.results, TRUE);
    }
  }

  g_free(search_text);
  g_free(file_path);
}

void app_main(GtkApplication *app, gpointer user_data) {
  (void)user_data;
  gtk_init();
  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window),
                       "Arbitrary Search spiritual successor");
  gtk_window_set_default_size(GTK_WINDOW(window), 320, 240);

  GtkWidget *outer_layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  GtkWidget *frame = gtk_frame_new("Search Settings");
  GtkWidget *search_settings_grid = gtk_grid_new();

  // ROM picker box
  GtkWidget *rom_path_label = gtk_label_new_with_mnemonic("_ROM Path:");
  gtk_widget_set_halign(rom_path_label, GTK_ALIGN_END);
  gtk_grid_attach(GTK_GRID(search_settings_grid), rom_path_label, 0, 0, 1, 1);

  GtkWidget *rom_path_entry = gtk_entry_new();
  gtk_widget_set_hexpand(rom_path_entry, TRUE);
  gtk_entry_set_activates_default(GTK_ENTRY(rom_path_entry), TRUE);
  gtk_grid_attach(GTK_GRID(search_settings_grid), rom_path_entry, 1, 0, 1, 1);

  struct browse_clicked_data *browse_clicked_data =
      g_malloc(sizeof(struct browse_clicked_data));
  browse_clicked_data->window = GTK_WINDOW(window);
  browse_clicked_data->entry = GTK_ENTRY(rom_path_entry);
  browse_clicked_data->next = NULL;

  GtkWidget *rom_path_browse_button = gtk_button_new_with_mnemonic("_Browse");
  gtk_grid_attach(GTK_GRID(search_settings_grid), rom_path_browse_button, 2, 0,
                  1, 1);
  g_signal_connect_data(rom_path_browse_button, "clicked",
                        G_CALLBACK(browse_clicked), browse_clicked_data,
                        free_from_closure, 0);

  gtk_label_set_mnemonic_widget(GTK_LABEL(rom_path_label), rom_path_entry);

  // Search string box
  GtkWidget *search_string_label =
      gtk_label_new_with_mnemonic("S_earch string:");
  gtk_widget_set_halign(search_string_label, GTK_ALIGN_END);
  gtk_grid_attach(GTK_GRID(search_settings_grid), search_string_label, 0, 1, 1,
                  1);

  GtkWidget *search_string_entry = gtk_search_entry_new();
  gtk_widget_set_hexpand(search_string_entry, TRUE);
  GValue value = {0};
  g_value_init(&value, G_TYPE_BOOLEAN);
  g_value_set_boolean(&value, TRUE);
  g_object_set_property(G_OBJECT(search_string_entry), "activates-default",
                        &value);
  gtk_grid_attach(GTK_GRID(search_settings_grid), search_string_entry, 1, 1, 2,
                  1);

  browse_clicked_data->next = search_string_entry;

  gtk_label_set_mnemonic_widget(GTK_LABEL(search_string_label),
                                search_string_entry);

  // Search button
  GtkWidget *search_button = gtk_button_new_with_mnemonic("_Search");
  gtk_grid_attach(GTK_GRID(search_settings_grid), search_button, 0, 2, 3, 1);

  struct search_clicked_data *search_clicked_data =
      g_malloc(sizeof(struct search_clicked_data));

  search_clicked_data->file_entry = GTK_EDITABLE(rom_path_entry);
  search_clicked_data->search_entry = GTK_EDITABLE(search_string_entry);

  g_signal_connect_data(search_button, "clicked", G_CALLBACK(search_clicked),
                        search_clicked_data, free_from_closure, 0);

  gtk_frame_set_child(GTK_FRAME(frame), search_settings_grid);
  gtk_box_append(GTK_BOX(outer_layout), frame);

  // Search results model
  GListStore *search_results_model = g_list_store_new(ASSS_TYPE_SEARCH_RESULT);
  search_clicked_data->results_store = search_results_model;
  GtkSingleSelection *search_results_selection_model =
      gtk_single_selection_new(G_LIST_MODEL(search_results_model));

  // dummy items to test with
  AsssSearchResult *search_result = asss_search_result_new(0x123, "test");
  g_list_store_append(search_results_model, search_result);
  g_object_unref(search_result);

  search_result = asss_search_result_new(0x456, "test2");
  g_list_store_append(search_results_model, search_result);
  g_object_unref(search_result);

  // Search results box
  GtkWidget *search_results =
      gtk_column_view_new(GTK_SELECTION_MODEL(search_results_selection_model));
  gtk_widget_set_vexpand(search_results, TRUE);

  // Address column
  GtkListItemFactory *address_column_factory =
      asss_make_address_column_factory();
  GtkColumnViewColumn *address_column =
      gtk_column_view_column_new("Address", address_column_factory);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(search_results),
                                address_column);

  // Result contents column
  GtkListItemFactory *result_column_factory = asss_make_result_column_factory();
  GtkColumnViewColumn *result_column =
      gtk_column_view_column_new("Result", result_column_factory);
  gtk_column_view_column_set_expand(result_column, TRUE);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(search_results), result_column);

  GtkWidget *search_results_scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(search_results_scroll),
                                search_results);
  gtk_box_append(GTK_BOX(outer_layout), search_results_scroll);

  // Table saving options
  GtkWidget *search_save_layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  GtkWidget *save_format_label = gtk_label_new_with_mnemonic("_Format:");
  gtk_box_append(GTK_BOX(search_save_layout), save_format_label);

  GtkListStore *save_format_model =
      gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  GtkTreeIter tree_append_iter;
  gtk_list_store_append(save_format_model, &tree_append_iter);
  gtk_list_store_set(save_format_model, &tree_append_iter, 0, "C99 arrays", 1,
                     0, -1);
  gtk_list_store_append(save_format_model, &tree_append_iter);
  gtk_list_store_set(save_format_model, &tree_append_iter, 0, "C++ arrays", 1,
                     1, -1);
  gtk_list_store_append(save_format_model, &tree_append_iter);
  gtk_list_store_set(save_format_model, &tree_append_iter, 0, "Python dicts", 1,
                     2, -1);

  GtkWidget *save_format_combo =
      gtk_combo_box_new_with_model(GTK_TREE_MODEL(save_format_model));
  g_object_unref(save_format_model);
  gtk_widget_set_hexpand(save_format_combo, TRUE);

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(save_format_combo), renderer,
                             TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(save_format_combo), renderer,
                                 "text", 0, NULL);

  gtk_combo_box_set_active(GTK_COMBO_BOX(save_format_combo), 0);

  gtk_box_append(GTK_BOX(search_save_layout), save_format_combo);

  gtk_label_set_mnemonic_widget(GTK_LABEL(save_format_label),
                                save_format_combo);

  GtkWidget *save_button = gtk_button_new_with_mnemonic("S_ave (not implemented yet)");
  gtk_box_append(GTK_BOX(search_save_layout), save_button);

  gtk_box_append(GTK_BOX(outer_layout), search_save_layout);

  // Not implemented yet, so disable
  gtk_widget_set_sensitive(save_button, FALSE);

  gtk_window_set_child(GTK_WINDOW(window), outer_layout);
  gtk_window_set_default_widget(GTK_WINDOW(window), search_button);
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
  GtkApplication *app =
      gtk_application_new("score.asss", G_APPLICATION_FLAGS_NONE);

  g_signal_connect(app, "activate", G_CALLBACK(app_main), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);

  g_object_unref(app);
  return status;
}
