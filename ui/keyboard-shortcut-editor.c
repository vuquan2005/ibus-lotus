#include <ctype.h>
#include <gtk/gtk.h>
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

char *fix_fb_alert = "Bật tùy chọn này nếu bạn gặp tình trạng lặp chữ khi chat trong Facebook, Messenger.\n\
Lưu ý: Tính năng này có thể khiến thanh địa chỉ trên trình duyệt Google Chrome hoạt động không chính xác.";
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
  saveShortcuts(key_pairs_tmp, 10);
  close_window_cb(widget, data);
}

void btn_macro_save_cb(GtkWidget *widget, gpointer data) {
  GtkTextBuffer *buffer = g_object_get_data(G_OBJECT(widget), "buffer");
  int nSaveMacroText = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "saveMacroText"));
  gchar *text;
  GtkTextIter start, end;
  gtk_text_buffer_get_bounds (buffer, &start, &end);

  text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
  if (nSaveMacroText) {
    saveMacroText(text);
  } else {
    saveConfigText(text);
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
  // GtkWidget *check;
  int pad = 10;
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

  /* --- Pack the checkbox into the parent (expand? fill? padding?).  --- */
  gtk_box_pack_start(GTK_BOX(parent), maskWidgets[mask_pos], FALSE, FALSE, pad);

  g_signal_connect(maskWidgets[mask_pos], "toggled", G_CALLBACK(check_event_cb),
                   GINT_TO_POINTER(mask_pos));
}

void add_macro_text(GtkWidget *widget, GtkWidget *w, char *macro_text, int saveMacroText) {
  GtkWidget *save_button, *macro_tv;
  GtkWidget *hbox;
  /* Horizontal box to pack save button */
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  GtkTextBuffer *buffer;
  macro_tv = gtk_text_view_new ();
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (macro_tv));

  gtk_text_buffer_set_text (buffer, macro_text, -1);
  gtk_container_add(GTK_CONTAINER(scrolled_window), macro_tv);
  gtk_scrolled_window_set_propagate_natural_width(GTK_SCROLLED_WINDOW(scrolled_window), 1);
  gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scrolled_window), 1);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(macro_tv), 30);

  gtk_widget_set_valign(hbox, GTK_ALIGN_END);
  gtk_widget_set_vexpand(hbox, TRUE);
  gtk_widget_set_halign(hbox, GTK_ALIGN_END);
  /* --- Pack it in. --- */
  gtk_box_pack_start(GTK_BOX(widget), scrolled_window, FALSE, FALSE, 0);
  /* --- Create a Save button. --- */
  save_button = gtk_button_new_with_label("Save");
  g_object_set_data(G_OBJECT(save_button), "buffer", buffer);
  g_object_set_data(G_OBJECT(save_button), "saveMacroText", GINT_TO_POINTER(saveMacroText));
  g_signal_connect(save_button, "clicked", G_CALLBACK(btn_macro_save_cb), w);
  /* --- Pack the button into the vertical box (vbox box1).  --- */
  gtk_box_pack_start(GTK_BOX(hbox), save_button, FALSE, FALSE, 10);
  gtk_widget_set_margin_bottom(hbox, 10);

  gtk_box_pack_start(GTK_BOX(widget), hbox, TRUE, TRUE, 0);
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
  GtkWidget *hbox, *label_hbox;
  GtkWidget *label;
  int pad = 10;
  /* Horizontal box to pack shortcut and label */
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  /* Horizontal box to pack label */
  label_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  /* --- create a new label.  --- */
  label = gtk_label_new(text);
  gtk_label_set_xalign(GTK_LABEL(label), 0);
  /* --- Pack the label into the horizontal box (expand? fill? padding)  --- */
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, pad);

  for (int i = 0; i < TOTAL_MASKS_PER_ROW; i++) {
    add_checkbox(hbox, labels[i], row * TOTAL_MASKS_PER_ROW + i);
  }

  /* --- Create an entry field --- */
  keyWidgets[row] = gtk_entry_new();
  GtkWidget *entry = keyWidgets[row];

  /* --- Pack the entry into the vertical box (expand? fill?, padding?).  --- */
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 10);

  /* --- Put some text in the field. --- */
  int idx = get_shortcut_pair_idx(row);
  int kvl = gdk_keyval_to_lower(key_pairs_tmp[idx+1]);
  gtk_entry_set_text(GTK_ENTRY(entry), int_to_accel(kvl));
  gtk_entry_set_alignment(GTK_ENTRY(entry), 0.5);

  /* --- Pack it in. --- */
  gtk_box_pack_start(GTK_BOX(widget), hbox, FALSE, FALSE, 0);

  g_signal_connect(entry, "key_press_event", G_CALLBACK(key_press_cb),
                   GINT_TO_POINTER(row));
  g_signal_connect(entry, "key_release_event", G_CALLBACK(key_release_cb),
                   GINT_TO_POINTER(row));
}

void add_control_buttons(GtkWidget *widget, GtkWidget *dialog) {
  GtkWidget *save_button;
  GtkWidget *cancel_button;
  GtkWidget *reset_button;
  GtkWidget *hbox;

  /* Horizontal box to pack OK and Cancel buttons */
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_halign(hbox, GTK_ALIGN_END);

  /* --- Create a Reset button. --- */
  reset_button = gtk_button_new_with_label("Reset");

  /* --- Pack the reset_button into the vertical box (vbox box1).  --- */
  gtk_box_pack_start(GTK_BOX(hbox), reset_button, FALSE, FALSE, 10);

  /* --- Create a Cancel button. --- */
  cancel_button = gtk_button_new_with_label("Cancel");

  /* --- Pack the cancel_button into the vertical box (vbox box1).  --- */
  gtk_box_pack_start(GTK_BOX(hbox), cancel_button, FALSE, FALSE, 10);

  /* --- Create a Save button. --- */
  save_button = gtk_button_new_with_label("Save");

  /* --- Pack the button into the vertical box (vbox box1).  --- */
  gtk_box_pack_start(GTK_BOX(hbox), save_button, FALSE, FALSE, 10);

  gtk_container_add(GTK_CONTAINER(widget), hbox);

  g_signal_connect(reset_button, "clicked", G_CALLBACK(btn_reset_cb), "clicked");
  g_signal_connect(save_button, "clicked", G_CALLBACK(btn_save_cb), dialog);
  g_signal_connect(cancel_button, "clicked", G_CALLBACK(close_window_cb),
                   dialog);
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
    // Returning TRUE means we handled the event, so the signal
    // emission should be stopped (don’t call any further callbacks
    // that may be connected). Return FALSE to continue invoking callbacks.
    return TRUE;
}


/*
 * Main - program begins here
 */
int openGUI(guint flags, int mode, guint32 *s, int size, char *mtext, char *cfg_text) {
  GtkWidget *w;
  GtkWidget *vbox, *vcbox;
  int which;
  int pad = 10;
  int arr[10] = {0};

  key_pairs_tmp = s;

  gtk_init(NULL, NULL);
  /* --- Create the top level window --- */
  w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request(w, 600, 150);

  /* --- You should always remember to connect the delete_event
   *     to the main window.
   */
  g_signal_connect(w, "delete_event", G_CALLBACK(close_window_cb), w);

  /* --- Give the window a border --- */
  gtk_container_set_border_width(GTK_CONTAINER(w), 2);

  /* --- We create a vertical box (vbox) to pack
   *     the horizontal boxes into.
   */
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, pad);

  for (int i = 0; i < TOTAL_ROWS; i++) {
    add_shortcut_box(vbox, text_arr[i], i);
  }

  vcbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, pad);
  add_control_buttons(vcbox, w);

  /* --- Align the controls box to the bottom.   --- */
  gtk_widget_set_valign(vcbox, GTK_ALIGN_END);
  gtk_widget_set_vexpand(vcbox, TRUE);
  gtk_box_pack_start(GTK_BOX(vbox), vcbox, TRUE, TRUE, 0);

  set_margin(vbox, 5, pad);


  GtkWidget *m_notebook;
    m_notebook = gtk_notebook_new();

    gtk_container_add(GTK_CONTAINER (w), m_notebook);

    GtkWidget *button;

    GtkWidget* keyboardPage = gtk_label_new("Phím tắt");
    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), vbox, keyboardPage);

    GtkWidget* macroPage = gtk_label_new("Gõ tắt");
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, pad);
    add_macro_text(vbox, w, mtext, 1);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), vbox, macroPage);

    GtkWidget* cfgPage = gtk_label_new("Tự định nghĩa kiểu gõ");
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, pad);
    add_macro_text(vbox, w, cfg_text, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), vbox, cfgPage);


  /*
   * --- Make the main window visible
   */
  gtk_window_set_title(GTK_WINDOW(w), "Settings");

  gtk_widget_show_all(GTK_WIDGET(w));

  gtk_main();
}

