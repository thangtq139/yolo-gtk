#include <gtk/gtk.h>
#include <darknet.h>
#include "attribute.h"

GtkBuilder *g_builder;

struct image_data {
	GtkImage *image;
	int rows, cols, stride;
};

void print_hello(GtkWidget *widget, gpointer data)
{
	g_print("Hello World\n");
}

// void show_unload_weight(GtkWidget *widget, gpointer window)
// {
// 	GtkWidget *dialog;
// 	dialog = gtk_builder_get_object(g_builder, "dialog");
// 	gtk_dialog_run(GTK_DIALOG(dialog));

// }

void free_bitmap(guchar *im_data, gpointer data)
{
	free(im_data);
}

void slide_show(char *file_name, gpointer parent_window)
{
	int i, j, k, src_index, dst_index;
	image im = load_image_color(file_name, 0, 0);
	g_printerr("Resolution: [%dx%d]; Channel: [%d]\n", im.w, im.h, im.c);
	struct image_data id;
	id.rows = im.h;
	id.cols = im.w;
	id.stride = id.cols * im.c;
	id.stride += (4 - (id.stride & 3));
	g_printerr("[Padding] = %d\n", id.stride);
	guchar *im_data = calloc(1, id.rows * id.stride);

	for (k = 0; k < im.c; ++k) {
		for (j = 0; j < im.h; ++j) {
			for (i = 0; i < im.w; ++i) {
				src_index = i + im.w * j + im.w * im.h * k;
				dst_index = k + im.c * i + id.stride * j;
				im_data[dst_index] = (guchar)(255 * im.data[src_index]);
			}
		}
	}

	GdkPixbuf *pb = gdk_pixbuf_new_from_data(im_data, GDK_COLORSPACE_RGB,
						 0, 8, id.cols, id.rows, id.stride,
						 free_bitmap, NULL);

	id.image = GTK_IMAGE(gtk_image_new_from_pixbuf(pb));

	GtkWidget *window;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), file_name);
	gtk_window_set_default_size(GTK_WINDOW(window), im.h, im.w);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(id.image));
	gtk_widget_show_all(window);
}

void select_image(GtkWidget *widget, gpointer parent_window)
{
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;
	char *file_name = NULL;

	dialog = gtk_file_chooser_dialog_new("Select image",
					     GTK_WINDOW(parent_window),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     "Cancel",
					     GTK_RESPONSE_CANCEL,
					     "Open",
					     GTK_RESPONSE_ACCEPT,
					     NULL);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		file_name = gtk_file_chooser_get_filename(chooser);
		g_printerr("Detecting object(s) in file \"%s\"\n", file_name);
	}
	gtk_widget_destroy(dialog);

	if (file_name) {
		slide_show(file_name, parent_window);
		g_free(file_name);
	}

}

void get_weight(GtkWidget *widget, gpointer parent_window)
{
	GtkWidget *dialog;
	gint res;

	dialog = gtk_file_chooser_dialog_new("Open File",
					     GTK_WINDOW(parent_window),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     "Cancel",
					     GTK_RESPONSE_CANCEL,
					     "Open",
					     GTK_RESPONSE_ACCEPT,
					     NULL);

	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *file_name;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		file_name = gtk_file_chooser_get_filename(chooser);
		g_print("File choosen is: %s\n", file_name);
		g_free(file_name);
	}
	gtk_widget_destroy(dialog);
}

void fun(GtkWidget *widget, gpointer parent_window)
{
	g_printerr("[parent] = %ld\n", parent_window);
	GString *buf;
	buf = g_string_new(NULL);
	gint w, h;
	gtk_window_get_size(GTK_WINDOW(parent_window), &w, &h);
	g_string_printf(buf, "%d, %d", w + 5, h + 5);
	gtk_window_set_title(GTK_WINDOW(parent_window), buf->str);
	gtk_window_resize(GTK_WINDOW(parent_window), w + 5, h + 5);
	g_string_free(buf, TRUE);
}

int main(int argc, char *argv[])
{
	GObject *window;
	GObject *button;
	GError *error = NULL;

	gtk_init(&argc, &argv);

	g_builder = gtk_builder_new();
	
	g_printerr("Loading UI design from file: %s\n", UI_MAIN_FILE);
	if (gtk_builder_add_from_file(g_builder, UI_MAIN_FILE, &error) == 0) {
		g_printerr("%s\n", error->message);
		g_printerr("Load UI design from string\n");
		g_clear_error(&error);
		if (gtk_builder_add_from_string(g_builder, UI_MAIN_STR, -1, &error) == 0) {
			g_printerr("%s\n", error->message);
			g_clear_error(&error);
			return 1;
		}
	}

	window = gtk_builder_get_object(g_builder, "window_main");
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	button = gtk_builder_get_object(g_builder, "button_select_image");
	g_signal_connect(button, "clicked", G_CALLBACK(select_image), G_OBJECT(window));

	g_printerr("[window] = %ld\n", window);
	button = gtk_builder_get_object (g_builder, "button_quit");
	g_signal_connect(button, "clicked", G_CALLBACK(fun), window);

	gtk_main ();

	return 0;
}
