#include <gtk/gtk.h>
#include "attribute.h"

GtkBuilder *g_builder;

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

void get_weight(GtkWidget *widget, gpointer parent_window)
{
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
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

	window = gtk_builder_get_object (g_builder, "window");
	g_signal_connect(window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

	button = gtk_builder_get_object(g_builder, "button1");
	g_signal_connect(button, "clicked", G_CALLBACK (get_weight), NULL);

	button = gtk_builder_get_object (g_builder, "button2");
	g_signal_connect(button, "clicked", G_CALLBACK (print_hello), NULL);

	button = gtk_builder_get_object (g_builder, "quit");
	g_signal_connect(button, "clicked", G_CALLBACK (gtk_main_quit), NULL);

	gtk_main ();

	return 0;
}
