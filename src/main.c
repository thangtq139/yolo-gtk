#include <gtk/gtk.h>
#include <darknet.h>
#include "attribute.h"

GtkBuilder *g_builder;

char **class_names;
image **alphabets;
network *net;

image buff[3];
image buff_letter[3];
int buff_index = 0;
CvCapture *cap;
IplImage *ipl;
float fps = 0;
int running = 0;
int demo_frame = 3;
int demo_index = 0;
float **predictions;
float *avg;
int video_done = 0;
int demo_total = 0;

void show_image_cv(image p, const char *name, IplImage *disp);

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

int size_network(network *net)
{
	int i, count = 0;
	for (i = 0; i < net->n; ++i)
	{
		layer l = net->layers[i];
		if (l.type == YOLO || l.type == REGION || l.type == DETECTION)
			count += l.outputs;
	}
	return count;
}

void remember_network(network *net)
{
	int i;
	int count = 0;
	for (i = 0; i < net->n; ++i)
	{
		layer l = net->layers[i];
		if (l.type == YOLO || l.type == REGION || l.type == DETECTION) {
			memcpy(predictions[demo_index] + count,
				net->layers[i].output,
				sizeof(float) * l.outputs);
			count += l.outputs;
		}
	}
}

detection *avg_predictions(network *net, int *nboxes)
{
	int i, j;
	int count = 0;
	fill_cpu(demo_total, 0, avg, 1);
	for (j = 0; j < demo_frame; ++j) {
		axpy_cpu(demo_total, 1. / demo_frame, predictions[j], 1, avg, 1);
	}
	for (i = 0; i < net->n; ++i) {
		layer l = net->layers[i];
		if (l.type == YOLO || l.type == REGION || l.type == DETECTION) {
			memcpy(l.output, avg + count, sizeof(float) * l.outputs);
			count += l.outputs;
		}
	}
	detection *dets = get_network_boxes(net, buff[0].w, buff[0].h, .5, .5, 0, 1, nboxes);
	return dets;
}

void *detect_in_thread(void *ptr)
{
	running = 1;
	float nms = .45;

	layer l = net->layers[net->n - 1];
	float *X = buff_letter[(buff_index + 2) % 3].data;
	network_predict(net, X);

	remember_network(net);
	detection *dets = 0;
	int nboxes  = 0;
	dets = avg_predictions(net, &nboxes);

	do_nms_obj(dets, nboxes, l.classes, nms);
	image display = buff[(buff_index + 2) % 3];
	draw_detections(display, dets, nboxes, .5, class_names, alphabets, l.classes);
	free_detections(dets, nboxes);
	demo_index = (demo_index + 1) % demo_frame;
	running = 0;
	return 0;
}

void *fetch_in_thread(void *ptr)
{
	int status = fill_image_from_stream(cap, buff[buff_index]);
	letterbox_image_into(buff[buff_index], net->w, net->h, buff_letter[buff_index]);
	if (status == 0)
		video_done = 1;
	return 0;
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

void *display_in_thread(void *ptr)
{
	show_image_cv(buff[(buff_index + 1) % 3], "Hello", ipl);
	cvWaitKey(100);
}

void video_show(char *file_name, gpointer parent_window)
{
	pthread_t detect_thread;
	pthread_t fetch_thread;

	int i;
	demo_total = size_network(net);
	predictions = calloc(demo_frame, sizeof(float *));
	for (i = 0; i < demo_frame; ++i)
		predictions[i] = calloc(demo_total, sizeof(float));

	avg = calloc(demo_total, sizeof(float));

	cap = cvCaptureFromFile(file_name);

	if (!cap) {
		g_printerr("Could not open file: %s\n", file_name);
		return;
	}

	buff[0] = get_image_from_stream(cap);
	buff[1] = copy_image(buff[0]);
	buff[2] = copy_image(buff[0]);
	buff_letter[0] = letterbox_image(buff[0], net->w, net->h);
	buff_letter[1] = letterbox_image(buff[0], net->w, net->h);
	buff_letter[2] = letterbox_image(buff[0], net->w, net->h);
	ipl = cvCreateImage(cvSize(buff[0].w, buff[0].h), IPL_DEPTH_8U, buff[0].c);

	int count = 0;
	cvNamedWindow("Hello", CV_WINDOW_NORMAL);
	cvMoveWindow("Hello", 0, 0);
	cvResizeWindow("Hello", 1352, 1013);
	g_printerr("Init window completed\n");

	float time_start = what_time_is_it_now();

	while (!video_done) {
		g_printerr("%d\n", count);
		buff_index = (buff_index + 1) % 3;
		if (pthread_create(&fetch_thread, 0, fetch_in_thread, 0)) {
			g_printerr("Fetch image thread creation failed\n");
			return;
		}
		if (pthread_create(&detect_thread, 0, detect_in_thread, 0)) {
			g_printerr("Detection thread creation failed\n");
			return;
		}
		g_printerr("Create and fetch done\n");
		fps = 1. / (what_time_is_it_now() - time_start);
		time_start = what_time_is_it_now();
		display_in_thread(0);
		g_printerr("Show done\n");
		pthread_join(fetch_thread, 0);
		pthread_join(detect_thread, 0);
		++count;
	}
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

void select_video(GtkWidget *widget, gpointer parent_window)
{
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;
	char *file_name = NULL;

	dialog = gtk_file_chooser_dialog_new("Select video",
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
		g_printerr("Detecting object(s) in video \"%s\"\n", file_name);
	}
	gtk_widget_destroy(dialog);

	if (file_name) {
		video_show(file_name, parent_window);
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
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *button_select_video;
	GtkWidget *button_import_cfg;
	GtkWidget *button_select_image;
	GError *error = NULL;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 230, 250);
	gtk_window_set_title(GTK_WINDOW(window), "Yolo-Gui");
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);

	vbox = gtk_vbox_new(TRUE, 1);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	button_select_image = gtk_button_new_with_label("Select image");
	button_import_cfg = gtk_button_new_with_label("Import configure");
	button_select_video = gtk_button_new_with_label("Select video");


	gtk_box_pack_start(GTK_BOX(vbox), button_import_cfg, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), button_select_image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), button_select_video, TRUE, TRUE, 0);

	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(button_import_cfg, "clicked", G_CALLBACK(import_cfg), window);
	g_signal_connect(button_select_image, "clicked", G_CALLBACK(select_image), G_OBJECT(window));
	g_signal_connect(button_select_video, "clicked", G_CALLBACK(select_video), G_OBJECT(window));

	gtk_widget_show_all(window);
	gtk_main();

	// g_builder = gtk_builder_new();
	
	// g_printerr("Loading UI design from file: %s\n", UI_MAIN_FILE);
	// if (gtk_builder_add_from_file(g_builder, UI_MAIN_FILE, &error) == 0) {
	// 	g_printerr("%s\n", error->message);
	// 	g_printerr("Load UI design from string\n");
	// 	g_clear_error(&error);
	// 	if (gtk_builder_add_from_string(g_builder, UI_MAIN_STR, -1, &error) == 0) {
	// 		g_printerr("%s\n", error->message);
	// 		g_clear_error(&error);
	// 		return 1;
	// 	}
	// }

	// window = gtk_builder_get_object(g_builder, "window_main");
	// g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	// button = gtk_builder_get_object(g_builder, "button_select_image");
	// g_signal_connect(button, "clicked", G_CALLBACK(select_image), G_OBJECT(window));

	// button = gtk_builder_get_object (g_builder, "button_import_cfg");
	// g_signal_connect(button, "clicked", G_CALLBACK(import_cfg), window);

	// button = gtk_builder_get_object(g_builder, "button_select_video");
	// g_signal_connect(button, "clicked", G_CALLBACK(select_video), G_OBJECT(window));

	// gtk_main ();

	return 0;
}
