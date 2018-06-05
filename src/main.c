#include <gtk/gtk.h>
#include <darknet.h>
#include "attribute.h"

GtkBuilder *g_builder;

char **class_names;
image **alphabets;
network *net;

struct image_data {
	GtkImage *image;
	int rows, cols, stride;
};

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

struct image_data *get_displayable_image(image *im)
{
	int i, j, k, src_index, dst_index;
	struct image_data *id = calloc(1, sizeof(struct image_data));
	id->rows = im->h;
	id->cols = im->w;
	id->stride = id->cols * im->c;
	id->stride += (4 - (id->stride & 3));
	g_printerr("[Padding] = %d\n", id->stride);
	guchar *im_data = calloc(1, id->rows * id->stride);

	for (k = 0; k < im->c; ++k) {
		for (j = 0; j < im->h; ++j) {
			for (i = 0; i < im->w; ++i) {
				src_index = i + im->w * j + im->w * im->h * k;
				dst_index = k + im->c * i + id->stride * j;
				im_data[dst_index] = (guchar)(255 * im->data[src_index]);
			}
		}
	}

	GdkPixbuf *pb = gdk_pixbuf_new_from_data(im_data, GDK_COLORSPACE_RGB,
						 0, 8, id->cols, id->rows, id->stride,
						 free_bitmap, NULL);

	id->image = GTK_IMAGE(gtk_image_new_from_pixbuf(pb));

	return id;
}

void slide_show(char *file_name, gpointer parent_window)
{
	image im = load_image_color(file_name, 0, 0);
	g_printerr("Resolution: [%dx%d]; Channel: [%d]\n", im.w, im.h, im.c);
	srand(2222222);
	double time;
	float nms = .45;
	image sized = letterbox_image(im, net->w, net->h);
	layer l = net->layers[net->n - 1];
	float *X = sized.data;
	time = what_time_is_it_now();
	network_predict(net, X);
	g_printerr("Prediction time: %f seconds.\n", what_time_is_it_now() - time);
	int nboxes = 0;
	detection *dets = get_network_boxes(net, im.w, im.h, .5, .5, 0, 1, &nboxes);
	if (nms) do_nms_sort(dets, nboxes, l.classes, nms);
	draw_detections(im, dets, nboxes, .5, class_names, alphabets, l.classes);
	free_detections(dets, nboxes);
	struct image_data *id = get_displayable_image(&im);

	GtkWidget *window;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), file_name);
	gtk_window_set_default_size(GTK_WINDOW(window), id->rows, id->cols);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(id->image));
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
		free(file_name);
	}
}

image **main_load_alphabet(const char *path)
{
	int i, j;
	const int nsize = 8;
	image **ret = calloc(nsize, sizeof(image));
	for (j = 0; j < nsize; ++j) {
		ret[j] = calloc(128, sizeof(image));
		for (i = 32; i < 127; ++i) {
			char buf[256];
			sprintf(buf, "%s%d_%d.png", path, i, j);
			ret[j][i] = load_image_color(buf, 0, 0);
		}
	}
	return ret;
}

char *str_concate(const char *s1, const char *s2)
{
	int n, m;
	char *ret;
	n = strlen(s1);
	m = strlen(s2);
	ret = malloc(m + n + 1);
	memcpy(ret, s1, n);
	memcpy(ret + n, s2, m);
	ret[m + n] = '\0';
	return ret;
}

char *get_base_path(const char *cmd)
{
	int n;
	char *ret;
	n = strlen(cmd);
	while (n && cmd[n - 1] != '/') --n;
	if (n == 0)
		return NULL;
	ret = malloc(n + 1);
	memcpy(ret, cmd, n);
	ret[n] = '\0';
	return ret;
}

void main_load_configure(char *cfg_file)
{
	extern char **class_names;
	extern image **alphabets;
	extern network *net;
	char *base_path = get_base_path(cfg_file);
	g_printerr("[base_path] = %s\n", base_path);
	list *options = read_data_cfg(cfg_file);

	char *names_file = str_concate(base_path,
			option_find_str(options, "names", "names.list"));
	g_printerr("[names] = %s\n", names_file);
	class_names = get_labels(names_file);

	char *alphabet_path = str_concate(base_path,
			option_find_str(options, "alphabet", "labels/"));
	g_printerr("[alphabet] = %s\n", alphabet_path);
	alphabets = main_load_alphabet(alphabet_path);

	char *weight_cfg = str_concate(base_path,
			option_find_str(options, "cfg", "weights.cfg"));
	char *weight_bin = str_concate(base_path,
			option_find_str(options, "weight", "weights.bin"));

	g_printerr("[weight_cfg] = %s\n", weight_cfg);
	g_printerr("[weight_bin] = %s\n", weight_bin);
	net = load_network(weight_cfg, weight_bin, 0);
	set_batch_network(net, 1);

	free(names_file);
	free(alphabet_path);
	free(weight_cfg);
	free(weight_bin);
}

void import_cfg(GtkWidget *widget, gpointer parent_window)
{
	GtkWidget *dialog;
	gint res;
	char *file_name = NULL;

	dialog = gtk_file_chooser_dialog_new("Import YOLO configure",
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
	}

	if (file_name) {
		main_load_configure(file_name);
		free(file_name);
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

	button = gtk_builder_get_object (g_builder, "button_import_cfg");
	g_signal_connect(button, "clicked", G_CALLBACK(import_cfg), window);

	gtk_main ();

	return 0;
}
