#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include "_cgo_export.h"

#define TOTAL_ROWS 2
#define TOTAL_MASKS_PER_ROW 4

// Go Config.Shortcuts has 10 elements (5 shortcut pairs)
#define TOTAL_SHORTCUT_KEYS 10

// Indices in key_pairs_tmp for each shortcut pair start
#define SHORTCUT_INPUT_MODE_SWITCH 0   // KSInputModeSwitch
#define SHORTCUT_RESTORE_KEY_STROKES 2 // KSRestoreKeyStrokes
#define SHORTCUT_VI_EN_SWITCH 4        // KSViEnSwitch (not used in UI anymore)
#define SHORTCUT_EMOJI_DIALOG 6        // KSEmojiDialog
#define SHORTCUT_HEXADECIMAL 8         // KSHexadecimal (deprecated)

int row = 0;
int col = 0;
const int KEYVAL = 1;
const int MASK = 0;
guint32 *key_pairs_tmp;

guint current_flags = 0;
guint current_ib_flags = 0;
GtkWidget *combo_im;
GtkWidget *combo_cs;
GtkWidget *chk_std_tone;
GtkWidget *chk_free_tone;
GtkWidget *chk_macro_enabled;
GtkWidget *chk_auto_capitalize;
GtkWidget *chk_spell_check;
GtkWidget *chk_spell_rules;
GtkWidget *chk_spell_dicts;
GtkWidget *chk_no_underline;

GtkWidget *chk_enable_hwnd;
GtkWidget *chk_enable_wmclass;
GtkWidget *chk_enable_toast;
GtkWidget *chk_enable_autoswitch;
GtkListStore *list_store;

// Global buffers to unify saving
GtkTextBuffer *macro_buffer = NULL;
GtkTextBuffer *cfg_buffer = NULL;

/*
 * get_shortcut_pair_idx
 *
 * Maps a GUI row index (0-1) to the corresponding key pair start index in
 * the configuration array (key_pairs_tmp), skipping SHORTCUT_VI_EN_SWITCH and SHORTCUT_EMOJI_DIALOG.
 */
int get_shortcut_pair_idx(int row) {
  if (row == 0) return SHORTCUT_INPUT_MODE_SWITCH;
  if (row == 1) return SHORTCUT_RESTORE_KEY_STROKES;
  return 0;
}

char *labels[TOTAL_MASKS_PER_ROW] = {"Ctrl", "Alt", "Shift", "Super"};
int masks[TOTAL_MASKS_PER_ROW] = {GDK_CONTROL_MASK, GDK_MOD1_MASK, GDK_SHIFT_MASK,
                         GDK_SUPER_MASK};
int keyvals[TOTAL_MASKS_PER_ROW] = {GDK_KEY_Control_L, GDK_KEY_Alt_L, GDK_KEY_Shift_L,
                           GDK_KEY_Super_L};
char *text_arr[TOTAL_ROWS] = {"Chuyển chế độ gõ", "Khôi phục phím"};
GtkWidget *maskWidgets[TOTAL_MASKS_PER_ROW * TOTAL_ROWS];
GtkWidget *keyWidgets[TOTAL_ROWS];
int usIM = 0;

/*
 * Destroy
 *
 * Close down the application
 */
gint close_window_cb(GtkWidget *widget, gpointer *dialog) {
  if (GTK_IS_WIDGET(dialog)) {
    gtk_widget_destroy(GTK_WIDGET(dialog));
  } else if (GTK_IS_WIDGET(widget)) {
    gtk_widget_destroy(GTK_WIDGET(widget));
  }
  gtk_main_quit();
  return FALSE;
}

gint btn_reset_cb(GtkWidget *widget, gpointer *data) {
  // Reset all 10 entries in the underlying array
  for (int i = 0 ; i < TOTAL_SHORTCUT_KEYS; i++ ){
    key_pairs_tmp[i] = 0;
  }
  for (int i=0 ; i < TOTAL_ROWS * TOTAL_MASKS_PER_ROW ; i++) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(maskWidgets[i]), FALSE);
  }
  for (int i=0 ; i < TOTAL_ROWS ; i++) {
    gtk_entry_set_text(GTK_ENTRY(keyWidgets[i]), "");
  }
  return FALSE;
}

/*
 * btn_save_cb
 *
 * Some event happened and the name is passed in the
 * data field.
 */
void btn_save_cb(GtkWidget *widget, gpointer data) {
  // Save config JSON only if the raw editor buffer was actually modified.
  if (cfg_buffer != NULL && gtk_text_buffer_get_modified(cfg_buffer)) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(cfg_buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(cfg_buffer, &start, &end, FALSE);
    saveConfigText(text);
    g_free(text);
  } else {
    // Otherwise, save the GUI shortcuts and options.
    saveShortcuts(key_pairs_tmp, 10);

    gchar *im = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_im));
    gchar *cs = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_cs));
    if (im == NULL) im = "";
    if (cs == NULL) cs = "";

    guint flags = 0;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_free_tone))) flags |= 1;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_std_tone))) flags |= 2;
    if (current_flags & 4) flags |= 4; // Preserve EautoCorrectEnabled

    guint ib_flags = 0;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_macro_enabled))) ib_flags |= 2;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_spell_check))) ib_flags |= 16;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_no_underline))) ib_flags |= 128;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_spell_rules))) ib_flags |= 256;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_spell_dicts))) ib_flags |= 512;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_auto_capitalize))) ib_flags |= 32768;

    // Preserve other flags in current_ib_flags
    guint mask_to_preserve = 1 | 32 | 64 | 1024 | 2048;
    ib_flags |= (current_ib_flags & mask_to_preserve);

    saveConfigOptions(flags, ib_flags, im, cs);

    int enableHwnd = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_enable_hwnd)) ? 1 : 0;
    int enableWmClass = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_enable_wmclass)) ? 1 : 0;
    int enableToast = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_enable_toast)) ? 1 : 0;
    int enableAutoSwitch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_enable_autoswitch)) ? 1 : 0;

    // Serialize tree view mappings
    GtkTreeIter list_iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &list_iter);
    GString *mapping_gstr = g_string_new("");
    gboolean first = TRUE;

    while (valid) {
      gchar *app_class = NULL;
      gchar *mode_name = NULL;
      gtk_tree_model_get(GTK_TREE_MODEL(list_store), &list_iter, 0, &app_class, 1, &mode_name, -1);

      if (app_class && strlen(app_class) > 0) {
        int mode_val = 1;
        if (g_strcmp0(mode_name, "Tiếng Việt (Surrounding Text)") == 0) {
          mode_val = 2;
        } else if (g_strcmp0(mode_name, "Tiếng Anh (Exclusion)") == 0) {
          mode_val = 3;
        }

        if (!first) {
          g_string_append(mapping_gstr, ",");
        }
        first = FALSE;
        g_string_append_printf(mapping_gstr, "%s:%d", app_class, mode_val);
      }

      if (app_class) g_free(app_class);
      if (mode_name) g_free(mode_name);

      valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &list_iter);
    }

    saveTrackingOptions(enableHwnd, enableWmClass, enableToast, enableAutoSwitch, mapping_gstr->str);
    g_string_free(mapping_gstr, TRUE);

    if (im != NULL && g_strcmp0(im, "") != 0) g_free(im);
    if (cs != NULL && g_strcmp0(cs, "") != 0) g_free(cs);
  }

  // Save macro text only if it was actually modified by the user.
  if (macro_buffer != NULL && gtk_text_buffer_get_modified(macro_buffer)) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(macro_buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(macro_buffer, &start, &end, FALSE);
    saveMacroText(text);
    g_free(text);
  }

  close_window_cb(widget, data);
}

/*
 * check_event_cb
 *
 * Handle a checkbox signal
 */
void check_event_cb(GtkWidget *widget, gpointer data) {
  int pos = GPOINTER_TO_INT(data);
  int row = pos / TOTAL_MASKS_PER_ROW, mask_col = pos % TOTAL_MASKS_PER_ROW;
  int idx = get_shortcut_pair_idx(row);
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    key_pairs_tmp[idx] |= masks[mask_col];
  } else {
    key_pairs_tmp[idx] &= ~masks[mask_col];
  }
}

char * int_to_accel(int keyval) {
  gchar *accel = NULL;
  accel = gtk_accelerator_get_label(keyval, 0);

  // Convert to upper case
  char *s = accel;
  while (*s) {
    *s = toupper((unsigned char)*s);
    s++;
  }
  return accel;
}

static gboolean key_release_cb(GtkWidget *entry, GdkEventKey *event,
                           gpointer data) {
  int row = GPOINTER_TO_INT(data);
  int idx = get_shortcut_pair_idx(row);
  int keyval = key_pairs_tmp[idx + 1];

  /* --- Put text in the field. --- */
  gtk_entry_set_text(GTK_ENTRY(entry), int_to_accel(keyval));
  return TRUE;
}

static gboolean key_press_cb(GtkWidget *entry, GdkEventKey *event, gpointer data) {
  int row = GPOINTER_TO_INT(data);
  int idx = get_shortcut_pair_idx(row);
  if (event->keyval == GDK_KEY_BackSpace || event->keyval == GDK_KEY_Delete) {
    key_pairs_tmp[idx + 1] = 0;
    return FALSE;
  }
  key_pairs_tmp[idx + 1] = gdk_keyval_to_lower(event->keyval);
  return TRUE;
}

void add_checkbox(GtkWidget *parent, char *text, int mask_pos) {
  int pad = 6;
  /*
   * --- Create a check button
   */
  maskWidgets[mask_pos] = gtk_check_button_new_with_label(text);
  /*
   * --- Active/Inactive check button
   */
  int row = mask_pos / TOTAL_MASKS_PER_ROW, mask_col = mask_pos % TOTAL_MASKS_PER_ROW;
  int idx = get_shortcut_pair_idx(row);
  int mask = key_pairs_tmp[idx];
  gboolean active = FALSE;
  if (mask&masks[mask_col]) {
    active = TRUE;
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(maskWidgets[mask_pos]), active);

  /* --- Pack the checkbox into the parent. --- */
  gtk_box_pack_start(GTK_BOX(parent), maskWidgets[mask_pos], FALSE, FALSE, pad);

  g_signal_connect(maskWidgets[mask_pos], "toggled", G_CALLBACK(check_event_cb),
                   GINT_TO_POINTER(mask_pos));
}

void add_macro_text(GtkWidget *widget, GtkWidget *w, char *macro_text, int saveMacroText) {
  GtkWidget *macro_tv;
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  GtkTextBuffer *buffer;
  macro_tv = gtk_text_view_new ();
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (macro_tv));

  gtk_text_buffer_set_text (buffer, macro_text, -1);
  gtk_text_buffer_set_modified (buffer, FALSE);
  gtk_container_add(GTK_CONTAINER(scrolled_window), macro_tv);
  gtk_scrolled_window_set_propagate_natural_width(GTK_SCROLLED_WINDOW(scrolled_window), 1);
  gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scrolled_window), 1);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(macro_tv), 10);

  // Setup layout flags to fill the container space
  gtk_widget_set_vexpand(scrolled_window, TRUE);
  gtk_widget_set_hexpand(scrolled_window, TRUE);
  gtk_box_pack_start(GTK_BOX(widget), scrolled_window, TRUE, TRUE, 0);

  // Track the text buffers for global saving
  if (saveMacroText) {
    macro_buffer = buffer;
  } else {
    cfg_buffer = buffer;
  }
}

static void
show_input_mode_alert (char  *msg)
{
  GtkWidget *dialog;
  dialog=gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s", msg);
  if(dialog)
  {
    g_signal_connect_swapped(dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);
    gtk_widget_show_all(dialog);
  }
}

void add_shortcut_box(GtkWidget *widget, char *text, int row) {
  GtkWidget *hbox;
  GtkWidget *label;
  
  // Wrap shortcut rows in nice modern card styled boxes
  GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  /* --- create a new label.  --- */
  label = gtk_label_new(text);
  gtk_label_set_xalign(GTK_LABEL(label), 0);
  gtk_style_context_add_class(gtk_widget_get_style_context(label), "card-title");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 5);

  for (int i = 0; i < TOTAL_MASKS_PER_ROW; i++) {
    add_checkbox(hbox, labels[i], row * TOTAL_MASKS_PER_ROW + i);
  }

  /* --- Create an entry field --- */
  keyWidgets[row] = gtk_entry_new();
  GtkWidget *entry = keyWidgets[row];
  gtk_style_context_add_class(gtk_widget_get_style_context(entry), "keycap");
  gtk_widget_set_size_request(entry, 80, -1);

  /* --- Pack the entry into the horizontal box.  --- */
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 10);

  /* --- Put some text in the field. --- */
  int idx = get_shortcut_pair_idx(row);
  int kvl = gdk_keyval_to_lower(key_pairs_tmp[idx+1]);
  gtk_entry_set_text(GTK_ENTRY(entry), int_to_accel(kvl));
  gtk_entry_set_alignment(GTK_ENTRY(entry), 0.5);

  gtk_container_add(GTK_CONTAINER(card), hbox);
  gtk_box_pack_start(GTK_BOX(widget), card, FALSE, FALSE, 0);

  g_signal_connect(entry, "key_press_event", G_CALLBACK(key_press_cb),
                   GINT_TO_POINTER(row));
  g_signal_connect(entry, "key_release_event", G_CALLBACK(key_release_cb),
                   GINT_TO_POINTER(row));
}

static void set_margin ( GtkWidget *vbox, gint hmargin, gint vmargin )
{
  gtk_widget_set_margin_start(vbox, hmargin);
  gtk_widget_set_margin_end(vbox, hmargin);
  gtk_widget_set_margin_top(vbox, vmargin);
  gtk_widget_set_margin_bottom(vbox, vmargin);
}

static gboolean
tooltip_press_callback (GtkWidget      *event_box,
                       GdkEventButton *event,
                       gpointer        data)
{
    g_print ("Event box clicked at coordinates %f,%f\n",
         event->x, event->y);
    show_input_mode_alert((char*)data);
    return TRUE;
}

static void on_spell_check_toggled(GtkToggleButton *button, gpointer data) {
  gboolean active = gtk_toggle_button_get_active(button);
  gtk_widget_set_sensitive(chk_spell_rules, active);
  gtk_widget_set_sensitive(chk_spell_dicts, active);
}

static void on_app_class_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data) {
  GtkTreeModel *model = GTK_TREE_MODEL(user_data);
  GtkTreeIter iter;
  if (gtk_tree_model_get_iter_from_string(model, &iter, path)) {
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, new_text, -1);
  }
}

static void on_mode_edited(GtkCellRendererCombo *renderer, gchar *path, gchar *new_text, gpointer user_data) {
  GtkTreeModel *model = GTK_TREE_MODEL(user_data);
  GtkTreeIter iter;
  if (gtk_tree_model_get_iter_from_string(model, &iter, path)) {
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, new_text, -1);
  }
}

static void on_btn_add_rule_clicked(GtkButton *button, gpointer user_data) {
  GtkListStore *store = GTK_LIST_STORE(user_data);
  GtkTreeIter iter;
  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, 0, "nhap.ten.app", 1, "Tiếng Việt (Pre-edit)", -1);
}

static void on_btn_delete_rule_clicked(GtkButton *button, gpointer user_data) {
  GtkTreeView *treeview = GTK_TREE_VIEW(user_data);
  GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
  GtkTreeModel *model;
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
  }
}

static void on_auto_switch_toggled(GtkToggleButton *button, gpointer data) {
  gboolean active = gtk_toggle_button_get_active(button);
  gtk_widget_set_sensitive(chk_enable_hwnd, active);
  gtk_widget_set_sensitive(chk_enable_wmclass, active);
  gtk_widget_set_sensitive(chk_enable_toast, active);
}

static void on_macro_enabled_toggled(GtkToggleButton *button, gpointer data) {
  gboolean active = gtk_toggle_button_get_active(button);
  gtk_widget_set_sensitive(chk_auto_capitalize, active);
}

static void apply_css(void) {
  GtkCssProvider *provider = gtk_css_provider_new();
  // We use alpha() channel for background and border overlays.
  // This is mathematically guaranteed to work on both Light and Dark themes
  // since it blends transparency over whatever background the theme provides,
  // preventing text-readability/color-inversion issues.
  gtk_css_provider_load_from_data(provider,
    "notebook {\n"
    "  border-top: 1px solid alpha(@theme_fg_color, 0.12);\n"
    "}\n"
    "notebook tab {\n"
    "  padding: 8px 16px;\n"
    "  font-weight: bold;\n"
    "}\n"
    "notebook tab:checked {\n"
    "  color: @theme_selected_bg_color;\n"
    "}\n"
    ".card {\n"
    "  background-color: alpha(@theme_fg_color, 0.03);\n"
    "  border: 1px solid alpha(@theme_fg_color, 0.08);\n"
    "  border-radius: 8px;\n"
    "  padding: 14px;\n"
    "  margin-bottom: 12px;\n"
    "}\n"
    ".card-title {\n"
    "  font-weight: bold;\n"
    "  font-size: 11pt;\n"
    "  margin-bottom: 8px;\n"
    "}\n"
    "entry.keycap {\n"
    "  font-family: 'Monospace', monospace;\n"
    "  font-weight: bold;\n"
    "  font-size: 11pt;\n"
    "  background-color: alpha(@theme_fg_color, 0.05);\n"
    "  border: 1px solid alpha(@theme_fg_color, 0.15);\n"
    "  border-radius: 6px;\n"
    "  padding: 6px;\n"
    "}\n"
    "textview text {\n"
    "  font-family: 'Monospace', monospace;\n"
    "  font-size: 11pt;\n"
    "  padding: 10px;\n"
    "}\n",
    -1, NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
    GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(provider);
}

/*
 * Main - program begins here
 */
int openGUI(
    guint flags,
    guint ibFlags,
    int mode,
    guint32 *s,
    int size,
    char *mtext,
    char *cfg_text,
    char *curIM,
    char *curCS,
    char *allIMs,
    char *allCSs,
    int enableHwnd,
    int enableWmClass,
    int enableToast,
    int enableAutoSwitch,
    char *appMappings
) {
  GtkWidget *w;
  GtkWidget *vbox;
  int pad = 10;

  key_pairs_tmp = s;
  current_flags = flags;
  current_ib_flags = ibFlags;

  macro_buffer = NULL;
  cfg_buffer = NULL;

  gtk_init(NULL, NULL);

  // Dynamic system-wide GNOME dark-theme synchronization setup
  gboolean prefer_dark = FALSE;
  GSettingsSchemaSource *schema_source = g_settings_schema_source_get_default();
  if (schema_source) {
    GSettingsSchema *schema = g_settings_schema_source_lookup(schema_source, "org.gnome.desktop.interface", TRUE);
    if (schema) {
      GSettings *settings = g_settings_new("org.gnome.desktop.interface");
      if (settings) {
        gchar *color_scheme = g_settings_get_string(settings, "color-scheme");
        if (color_scheme && g_strcmp0(color_scheme, "prefer-dark") == 0) {
          prefer_dark = TRUE;
        }
        if (color_scheme) g_free(color_scheme);
        g_object_unref(settings);
      }
      g_settings_schema_unref(schema);
    }
  }

  // Also check if current GTK legacy theme name contains "dark"
  GtkSettings *gtk_settings = gtk_settings_get_default();
  gchar *theme_name = NULL;
  g_object_get(gtk_settings, "gtk-theme-name", &theme_name, NULL);
  if (theme_name) {
    gchar *lower_theme = g_utf8_strdown(theme_name, -1);
    if (g_strrstr(lower_theme, "dark") != NULL) {
      prefer_dark = TRUE;
    }
    g_free(lower_theme);
    g_free(theme_name);
  }

  if (prefer_dark) {
    g_object_set(gtk_settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
  }

  // Load custom style settings to match desktop theme
  apply_css();

  /* --- Create the top level window --- */
  w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request(w, 680, 620);
  gtk_container_set_border_width(GTK_CONTAINER(w), 0);

  // Set up HeaderBar for modern title decoration
  GtkWidget *header = gtk_header_bar_new();
  gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
  gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Cấu hình Lotus");
  gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header), "Bộ gõ Tiếng Việt Lotus");
  gtk_window_set_titlebar(GTK_WINDOW(w), header);

  // Cancel button
  GtkWidget *btn_cancel = gtk_button_new_with_label("Hủy");
  g_signal_connect(btn_cancel, "clicked", G_CALLBACK(close_window_cb), w);
  gtk_header_bar_pack_start(GTK_HEADER_BAR(header), btn_cancel);

  // Reset button
  GtkWidget *btn_reset = gtk_button_new_with_label("Mặc định");
  g_signal_connect(btn_reset, "clicked", G_CALLBACK(btn_reset_cb), NULL);
  gtk_header_bar_pack_start(GTK_HEADER_BAR(header), btn_reset);

  // Save button
  GtkWidget *btn_save = gtk_button_new_with_label("Lưu");
  GtkStyleContext *save_context = gtk_widget_get_style_context(btn_save);
  gtk_style_context_add_class(save_context, "suggested-action");
  g_signal_connect(btn_save, "clicked", G_CALLBACK(btn_save_cb), w);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), btn_save);

  g_signal_connect(w, "delete_event", G_CALLBACK(close_window_cb), w);

  GtkWidget *m_notebook = gtk_notebook_new();
  gtk_container_add(GTK_CONTAINER(w), m_notebook);

  // --- Page 1: Phím tắt ---
  GtkWidget *keyboardPage = gtk_label_new("Phím tắt");
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, pad);
  set_margin(vbox, 16, 16);

  for (int i = 0; i < TOTAL_ROWS; i++) {
    add_shortcut_box(vbox, text_arr[i], i);
  }
  gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), vbox, keyboardPage);

  // --- Page 2: Cài đặt ---
  GtkWidget *settingsPage = gtk_label_new("Cài đặt");
  GtkWidget *settings_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, pad);
  set_margin(settings_vbox, 16, 16);

  // Card 1: Kiểu gõ & Bảng mã
  GtkWidget *card_im_cs = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_style_context_add_class(gtk_widget_get_style_context(card_im_cs), "card");

  GtkWidget *lbl_im_cs = gtk_label_new("Kiểu gõ & Bảng mã");
  gtk_widget_set_halign(lbl_im_cs, GTK_ALIGN_START);
  gtk_style_context_add_class(gtk_widget_get_style_context(lbl_im_cs), "card-title");
  gtk_box_pack_start(GTK_BOX(card_im_cs), lbl_im_cs, FALSE, FALSE, 0);

  GtkWidget *grid_im_cs = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid_im_cs), 8);
  gtk_grid_set_column_spacing(GTK_GRID(grid_im_cs), 12);

  GtkWidget *lbl_im = gtk_label_new("Kiểu gõ:");
  gtk_widget_set_halign(lbl_im, GTK_ALIGN_START);
  combo_im = gtk_combo_box_text_new();
  gtk_widget_set_hexpand(combo_im, TRUE);

  GtkWidget *lbl_cs = gtk_label_new("Bảng mã:");
  gtk_widget_set_halign(lbl_cs, GTK_ALIGN_START);
  combo_cs = gtk_combo_box_text_new();
  gtk_widget_set_hexpand(combo_cs, TRUE);

  gtk_grid_attach(GTK_GRID(grid_im_cs), lbl_im, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid_im_cs), combo_im, 1, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid_im_cs), lbl_cs, 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid_im_cs), combo_cs, 1, 1, 1, 1);

  gtk_box_pack_start(GTK_BOX(card_im_cs), grid_im_cs, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(settings_vbox), card_im_cs, FALSE, FALSE, 0);

  // Populate Kiểu gõ
  gchar **im_items = g_strsplit(allIMs, ",", -1);
  for (int i = 0; im_items[i] != NULL; i++) {
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_im), im_items[i]);
    if (g_strcmp0(im_items[i], curIM) == 0) {
      gtk_combo_box_set_active(GTK_COMBO_BOX(combo_im), i);
    }
  }
  g_strfreev(im_items);

  // Populate Bảng mã
  gchar **cs_items = g_strsplit(allCSs, ",", -1);
  for (int i = 0; cs_items[i] != NULL; i++) {
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_cs), cs_items[i]);
    if (g_strcmp0(cs_items[i], curCS) == 0) {
      gtk_combo_box_set_active(GTK_COMBO_BOX(combo_cs), i);
    }
  }
  g_strfreev(cs_items);

  // Card 2: Hành vi gõ & Chính tả
  GtkWidget *card_behavior = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_style_context_add_class(gtk_widget_get_style_context(card_behavior), "card");

  GtkWidget *lbl_behavior = gtk_label_new("Hành vi gõ & Chính tả");
  gtk_widget_set_halign(lbl_behavior, GTK_ALIGN_START);
  gtk_style_context_add_class(gtk_widget_get_style_context(lbl_behavior), "card-title");
  gtk_box_pack_start(GTK_BOX(card_behavior), lbl_behavior, FALSE, FALSE, 0);

  chk_free_tone = gtk_check_button_new_with_label("Bỏ dấu tự do");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_free_tone), (flags & 1) != 0);
  gtk_box_pack_start(GTK_BOX(card_behavior), chk_free_tone, FALSE, FALSE, 0);

  chk_std_tone = gtk_check_button_new_with_label("Dấu thanh chuẩn (òa, úy...)");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_std_tone), (flags & 2) != 0);
  gtk_box_pack_start(GTK_BOX(card_behavior), chk_std_tone, FALSE, FALSE, 0);

  chk_spell_check = gtk_check_button_new_with_label("Kiểm tra chính tả");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_spell_check), (ibFlags & 16) != 0);
  gtk_box_pack_start(GTK_BOX(card_behavior), chk_spell_check, FALSE, FALSE, 0);

  // Spell check sub-options
  GtkWidget *vbox_spell_sub = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(vbox_spell_sub, 24);

  chk_spell_rules = gtk_check_button_new_with_label("Kiểm tra bằng luật vần");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_spell_rules), (ibFlags & 256) != 0);
  gtk_box_pack_start(GTK_BOX(vbox_spell_sub), chk_spell_rules, FALSE, FALSE, 0);

  chk_spell_dicts = gtk_check_button_new_with_label("Kiểm tra bằng từ điển");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_spell_dicts), (ibFlags & 512) != 0);
  gtk_box_pack_start(GTK_BOX(vbox_spell_sub), chk_spell_dicts, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(card_behavior), vbox_spell_sub, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(settings_vbox), card_behavior, FALSE, FALSE, 0);

  g_signal_connect(chk_spell_check, "toggled", G_CALLBACK(on_spell_check_toggled), NULL);
  on_spell_check_toggled(GTK_TOGGLE_BUTTON(chk_spell_check), NULL);

  // Card 3: Gõ tắt & Hiển thị
  GtkWidget *card_macro = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_style_context_add_class(gtk_widget_get_style_context(card_macro), "card");

  GtkWidget *lbl_macro = gtk_label_new("Gõ tắt & Hiển thị");
  gtk_widget_set_halign(lbl_macro, GTK_ALIGN_START);
  gtk_style_context_add_class(gtk_widget_get_style_context(lbl_macro), "card-title");
  gtk_box_pack_start(GTK_BOX(card_macro), lbl_macro, FALSE, FALSE, 0);

  chk_macro_enabled = gtk_check_button_new_with_label("Bật gõ tắt");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_macro_enabled), (ibFlags & 2) != 0);
  gtk_box_pack_start(GTK_BOX(card_macro), chk_macro_enabled, FALSE, FALSE, 0);

  GtkWidget *vbox_macro_sub = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(vbox_macro_sub, 24);

  chk_auto_capitalize = gtk_check_button_new_with_label("Tự động viết hoa từ gõ tắt");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_auto_capitalize), (ibFlags & 32768) != 0);
  gtk_box_pack_start(GTK_BOX(vbox_macro_sub), chk_auto_capitalize, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(card_macro), vbox_macro_sub, FALSE, FALSE, 0);

  chk_no_underline = gtk_check_button_new_with_label("Ẩn gạch chân khi gõ nháp (pre-edit)");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_no_underline), (ibFlags & 128) != 0);
  gtk_box_pack_start(GTK_BOX(card_macro), chk_no_underline, FALSE, FALSE, 6);

  gtk_box_pack_start(GTK_BOX(settings_vbox), card_macro, FALSE, FALSE, 0);

  g_signal_connect(chk_macro_enabled, "toggled", G_CALLBACK(on_macro_enabled_toggled), NULL);
  on_macro_enabled_toggled(GTK_TOGGLE_BUTTON(chk_macro_enabled), NULL);

  gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), settings_vbox, settingsPage);

  // --- Page 3: Tự động chuyển chế độ ---
  GtkWidget *trackingPage = gtk_label_new("Tự động chuyển chế độ");
  GtkWidget *tracking_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, pad);
  set_margin(tracking_vbox, 16, 16);

  // Master checkbox
  chk_enable_autoswitch = gtk_check_button_new_with_label("Bật cơ chế tự động chuyển chế độ gõ");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_enable_autoswitch), enableAutoSwitch != 0);
  gtk_box_pack_start(GTK_BOX(tracking_vbox), chk_enable_autoswitch, FALSE, FALSE, 0);

  // Sub options card
  GtkWidget *card_tracking = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_style_context_add_class(gtk_widget_get_style_context(card_tracking), "card");

  chk_enable_hwnd = gtk_check_button_new_with_label("Nhớ chế độ gõ riêng cho từng cửa sổ (Window ID)");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_enable_hwnd), enableHwnd != 0);
  gtk_box_pack_start(GTK_BOX(card_tracking), chk_enable_hwnd, FALSE, FALSE, 0);

  chk_enable_wmclass = gtk_check_button_new_with_label("Áp dụng chế độ gõ theo loại ứng dụng (Application/wmclass)");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_enable_wmclass), enableWmClass != 0);
  gtk_box_pack_start(GTK_BOX(card_tracking), chk_enable_wmclass, FALSE, FALSE, 0);

  chk_enable_toast = gtk_check_button_new_with_label("Hiển thị thông báo chế độ gõ khi bắt đầu nhập liệu");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_enable_toast), enableToast != 0);
  gtk_box_pack_start(GTK_BOX(card_tracking), chk_enable_toast, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(tracking_vbox), card_tracking, FALSE, FALSE, 0);

  // List editor card
  GtkWidget *card_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_style_context_add_class(gtk_widget_get_style_context(card_list), "card");
  gtk_widget_set_vexpand(card_list, TRUE);
  gtk_widget_set_hexpand(card_list, TRUE);

  GtkWidget *lbl_list = gtk_label_new("Danh sách quy tắc ứng dụng (wmclass)");
  gtk_widget_set_halign(lbl_list, GTK_ALIGN_START);
  gtk_style_context_add_class(gtk_widget_get_style_context(lbl_list), "card-title");
  gtk_box_pack_start(GTK_BOX(card_list), lbl_list, FALSE, FALSE, 0);

  // Scrolled window for TreeView
  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(scrolled_window, TRUE);
  gtk_widget_set_hexpand(scrolled_window, TRUE);

  // Create list store
  list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

  // Populate list store
  if (appMappings && strlen(appMappings) > 0) {
    gchar **rules = g_strsplit(appMappings, ",", -1);
    for (int i = 0; rules[i] != NULL; i++) {
      gchar **kv = g_strsplit(rules[i], ":", 2);
      if (kv[0] != NULL && kv[1] != NULL) {
        int val = atoi(kv[1]);
        char *mode_name = "Tiếng Việt (Pre-edit)";
        if (val == 2) {
          mode_name = "Tiếng Việt (Surrounding Text)";
        } else if (val == 3) {
          mode_name = "Tiếng Anh (Exclusion)";
        }
        GtkTreeIter iter;
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter, 0, kv[0], 1, mode_name, -1);
      }
      g_strfreev(kv);
    }
    g_strfreev(rules);
  }

  // TreeView
  GtkWidget *treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);
  
  // Column 1: App Class
  GtkCellRenderer *renderer_app = gtk_cell_renderer_text_new();
  g_object_set(renderer_app, "editable", TRUE, NULL);
  g_signal_connect(renderer_app, "edited", G_CALLBACK(on_app_class_edited), list_store);
  GtkTreeViewColumn *col_app = gtk_tree_view_column_new_with_attributes("Tên ứng dụng (wmclass)", renderer_app, "text", 0, NULL);
  gtk_tree_view_column_set_resizable(col_app, TRUE);
  gtk_tree_view_column_set_expand(col_app, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col_app);

  // Column 2: Mode Name
  GtkCellRenderer *renderer_mode = gtk_cell_renderer_combo_new();
  GtkListStore *combo_store = gtk_list_store_new(1, G_TYPE_STRING);
  GtkTreeIter combo_iter;
  gtk_list_store_append(combo_store, &combo_iter);
  gtk_list_store_set(combo_store, &combo_iter, 0, "Tiếng Việt (Pre-edit)", -1);
  gtk_list_store_append(combo_store, &combo_iter);
  gtk_list_store_set(combo_store, &combo_iter, 0, "Tiếng Việt (Surrounding Text)", -1);
  gtk_list_store_append(combo_store, &combo_iter);
  gtk_list_store_set(combo_store, &combo_iter, 0, "Tiếng Anh (Exclusion)", -1);

  g_object_set(renderer_mode,
               "model", GTK_TREE_MODEL(combo_store),
               "text-column", 0,
               "has-entry", FALSE,
               "editable", TRUE,
               NULL);
  g_signal_connect(renderer_mode, "edited", G_CALLBACK(on_mode_edited), list_store);
  GtkTreeViewColumn *col_mode = gtk_tree_view_column_new_with_attributes("Chế độ gõ", renderer_mode, "text", 1, NULL);
  gtk_tree_view_column_set_resizable(col_mode, TRUE);
  gtk_tree_view_column_set_min_width(col_mode, 200);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col_mode);

  gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
  gtk_box_pack_start(GTK_BOX(card_list), scrolled_window, TRUE, TRUE, 0);

  // Buttons box for List Store
  GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *btn_add = gtk_button_new_with_label("Thêm quy tắc");
  g_signal_connect(btn_add, "clicked", G_CALLBACK(on_btn_add_rule_clicked), list_store);
  gtk_box_pack_start(GTK_BOX(btn_box), btn_add, FALSE, FALSE, 0);

  GtkWidget *btn_delete = gtk_button_new_with_label("Xóa quy tắc");
  g_signal_connect(btn_delete, "clicked", G_CALLBACK(on_btn_delete_rule_clicked), treeview);
  gtk_box_pack_start(GTK_BOX(btn_box), btn_delete, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(card_list), btn_box, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(tracking_vbox), card_list, TRUE, TRUE, 0);

  g_signal_connect(chk_enable_autoswitch, "toggled", G_CALLBACK(on_auto_switch_toggled), NULL);
  on_auto_switch_toggled(GTK_TOGGLE_BUTTON(chk_enable_autoswitch), NULL);

  gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), tracking_vbox, trackingPage);

  // --- Page 3: Gõ tắt ---
  GtkWidget* macroPage = gtk_label_new("Gõ tắt");
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, pad);
  set_margin(vbox, 8, 8);
  add_macro_text(vbox, w, mtext, 1);
  gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), vbox, macroPage);

  // --- Page 4: Tự định nghĩa kiểu gõ ---
  GtkWidget* cfgPage = gtk_label_new("Tự định nghĩa kiểu gõ");
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, pad);
  set_margin(vbox, 8, 8);
  add_macro_text(vbox, w, cfg_text, 0);
  gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), vbox, cfgPage);

  /* --- Make the main window visible --- */
  gtk_widget_show_all(GTK_WIDGET(w));
  gtk_main();

  return 0;
}
