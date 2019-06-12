#include "check.h"
#include "ssdToHdd.h"
static vector<int> *result = new vector<int>;

static int check_list[LIST_NUM] = {0, };

static void ok_button_press_event(GtkWidget *widget, gpointer data);

static void checkbutton_callback(GtkWidget *widget, long int num);

static void cancel_event(GtkWidget *widget, gpointer data);

static void init_list();

static void destroy_window(GtkWidget *widget, gpointer data);

vector<int> * printList(int argc, char* argv[], vector<pair<double,string>> const* list, int errorCode) {
	init_list();
	result->clear();

	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *label_information;
	GtkWidget *hbox_button;
	GtkWidget *button_cancel;
	GtkWidget *button_ok;

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "SSD to HDD");

	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(destroy_window), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(destroy_window), NULL);

	gtk_container_border_width(GTK_CONTAINER (window), 10);

	switch (errorCode) {
		case RECOMMEND:
		{
			label_information = gtk_label_new("The disk is full. It is recommended to move the folder to the SSD. Would you like to move it?");
			gtk_widget_show(label_information);

			vbox = gtk_vbox_new(FALSE, 0);

			gtk_container_add(GTK_CONTAINER(vbox), label_information);

			gtk_container_add(GTK_CONTAINER(window), vbox);

			string pathlist[10];
			off_t size[10];
			int l = 0;
			double weight[10];
			for(vector<pair<double,string>>::const_iterator iter=list->begin(); iter!=list->end(); iter++) {
				pathlist[l] = (*iter).second;
				struct stat stat;
				struct s2hData s2hdata;
				getInfo(pathlist[l], &stat, &s2hdata);
				size[l] = s2hdata.sizeOfDir;
				weight[l] = (*iter).first;
				l += 1;
				if(l >= LIST_NUM)
					break;
			}

			GtkWidget *hbox[l];

			for(int i=0; i<l; i++) {
				hbox[i] = list_label_frame(vbox, pathlist[i].c_str(), weight[i], size[i], i);
				gtk_widget_show(hbox[i]);
			}

			hbox_button = gtk_hbox_new(FALSE, 0);
			button_cancel = gtk_button_new_with_label("cancel");
			button_ok = gtk_button_new_with_label("ok");

			g_signal_connect(button_ok, "clicked", G_CALLBACK(ok_button_press_event), window);
			g_signal_connect(GTK_OBJECT(button_cancel), "clicked", G_CALLBACK(cancel_event), window);

			gtk_container_add(GTK_CONTAINER(hbox_button), button_cancel);
			gtk_container_add(GTK_CONTAINER(hbox_button), button_ok);
			gtk_container_add(GTK_CONTAINER(vbox), hbox_button);
			gtk_widget_show(hbox_button);
			gtk_widget_show(button_cancel);
			gtk_widget_show(button_ok);

			gtk_widget_show(vbox);

			gtk_widget_show(window);
			gtk_main();
			break;
		}
		default:
		{
			printf("something wrong...\n");
			//result->push_back(-1);
			break;
		}
	}

	for(vector<int>::iterator iter=result->begin(); iter!=result->end(); iter++) {
		printf("checked : %d\n", (*iter));
	}

	printf("end gui\n");

	return result;
}

GtkWidget *list_label_frame(GtkWidget *parent, const char *label_text, double weight, off_t label_size, long int num) {
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *last_time_label;
	GtkWidget *size_label;
	GdkColor color;
	color.red = 0x0000;
	color.green = 0x0000;
	color.blue = 0x0000;
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_set_size_request(hbox, WINDOW_WIDTH, WIDGET_HEIGHT);
	gtk_container_add(GTK_CONTAINER(parent), hbox);

	button = gtk_check_button_new();
	g_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(checkbutton_callback), (gpointer) num);
	gtk_widget_set_size_request(button, WIDGET_WIDTH, WIDGET_HEIGHT);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	//gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	
	label = gtk_label_new(label_text);
	gtk_widget_set_size_request(label, WINDOW_WIDTH*2, WIDGET_HEIGHT);
	//gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(hbox), label);
	
	//gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
	gtk_label_set_max_width_chars(GTK_LABEL(label), 40);

	char score[10];
	sprintf(score, "%lf", weight);

	last_time_label = gtk_label_new(score);
	gtk_widget_set_size_request(last_time_label, WIDGET_WIDTH, WIDGET_HEIGHT);
	gtk_container_add(GTK_CONTAINER(hbox), last_time_label);

	string unit;
	off_t value;
	if(label_size < 1024) {
		unit = "Bytes";
		value = label_size;
	}
	else if(label_size >= 1024 && label_size < 1024 * 1024) {
		unit = "KB";
		value = label_size / 1024;
	}
	else if(label_size >= 1024 * 1024 && label_size < (off_t)1024 * 1024 * 1024) {
		unit = "MB";
		value = label_size / (1024 * 1024);
	}
	else if(label_size >= (off_t)1024 * 1024 * 1024 && label_size < (off_t)1024 * 1024 * 1024 * 1024) {
		unit = "GB";
		value = label_size / (1024 * 1024 * 1024);
	}

	string result = to_string((long long)value) + unit;

	size_label = gtk_label_new(result.c_str());
	gtk_widget_set_size_request(label, WIDGET_WIDTH*3, WIDGET_HEIGHT);
	gtk_container_add(GTK_CONTAINER(hbox), size_label);

	gtk_widget_modify_bg(label, GTK_STATE_NORMAL, &color);

	gtk_widget_show(button);
	gtk_widget_show(label);
	gtk_widget_show(size_label);
	gtk_widget_show(last_time_label);	
	return hbox;
}


static void ok_button_press_event(GtkWidget *widget, gpointer data) {
	(void)widget;
	(void)data;
	for(int i=0; i<LIST_NUM; i++) {
		if(check_list[i]==1)
			result->push_back(i);
	}
	gtk_widget_hide((GtkWidget *)data);
	gtk_main_quit();
}

static void checkbutton_callback(GtkWidget *widget, long int num) {
	if(GTK_TOGGLE_BUTTON(widget)->active) {
		g_print("%ld : activated\n", num);
		check_list[num] = 1;
	} else {
		g_print("%ld : not activated\n", num);
		check_list[num] = 0;
	}
	printf("\n");
}

static void cancel_event(GtkWidget *widget, gpointer data) {
	(void)widget;
	(void)data;

	gtk_widget_hide((GtkWidget *) data);
	gtk_main_quit();
}

static void init_list() {
	result->clear();
	for(int i=0; i<LIST_NUM; i++) {
		check_list[i] = 0;
	}
}

static void destroy_window(GtkWidget *widget, gpointer data) {
	(void) data;
	gtk_widget_hide(widget);
}
