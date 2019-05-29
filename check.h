#include <gtk/gtk.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <utility>
#include <stdlib.h>
//#include "ssdToHdd.h"

#define RECOMMEND 1
#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 700
#define WIDGET_HEIGHT 40
#define WIDGET_WIDTH 20
#define LIST_NUM 10

using namespace std;

GtkWidget *list_label_frame(GtkWidget *parent, const char *label_text, long int num);

//call gui
vector<int> * printList(int argc, char* argv[], vector<pair<double,string>> const* list, int errorCode);