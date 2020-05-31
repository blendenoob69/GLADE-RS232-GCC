//#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
//#include <stdlib.h>
//#include <stdio.h>
#include <linux/serial.h>
//#include <asm/ioctls.h>
#include <fcntl.h>
//#include <linux/fb.h>


#include <gtk/gtk.h>

#define MATRIX_SET_UART_TYPE	0xe002


void on_xWindow_destroy(void);



typedef struct {
    GtkWidget *w_txt_view_main;			// Pointer to text view object
    GtkWidget *w_dlg_file_choose;       // Pointer to file chooser dialog box
    GtkTextBuffer *txt_buf_main;		// Pointer to text buffer
    GtkWidget *w_hlp_mnu_About;
    GtkWidget *w_dlg_about;

    GtkEntry *w_ntr_telegramm;

    GtkWidget *w_btn_send_telegramm;
    GtkWidget *w_btn_rs232_connect;

    GtkWidget *w_lbl_time;
    
	GtkComboBox *w_cbx_port_select;
	GtkComboBox *w_cbx_baud_select;
	GtkComboBox *w_cbx_mode_select;
	
	//GtkListBox *w_rx_tree_view;
	GtkTreeView *w_rx_tree_view;
	GtkListStore *w_rx_tree_store;
	GtkTreeSelection *w_rx_tree_select;
	//GtkListStore *w_rx_tree_col1;
} app_widgets;

typedef struct{
int port;
int mode;
int fd;
unsigned int baud;
gboolean ser_is_open;
} struc_serialport;


struc_serialport *serialport;

app_widgets     *widgets;


gboolean timer_handler(/*app_widgets *widgets*/);

//-------------------------------------------------
int  recv_data ()
{
	char buf[1024];// = "Ace Test for serial gtk+";
	GtkTreeIter iter;
	int i;
	
	if(serialport->fd == 0)
		return 0;

	i = read(serialport->fd,buf,1024);
	buf[i] = '\0';
	
	if(i > 0)
	{

	gtk_list_store_append (GTK_LIST_STORE (widgets->w_rx_tree_store), &iter);
	gtk_list_store_set (GTK_LIST_STORE(widgets->w_rx_tree_store), &iter,0, buf,-1);

	g_print("recv_data::\n%s\n",buf);
	}	

	return 1;
}

//baud=0..N == (B0, B50, B75, B110, ..B38400,..B460800) == || 0..38400..
void open_port()
{
	char devname[32];
	struct termios term;
	
	sprintf(devname,"/dev/ttyS%d",serialport->port);
	//fd = open(devname,  O_RDWR | O_NOCTTY | O_NDELAY);
//	serialport->fd = open("/dev/ttyACM0",  O_RDWR | O_NOCTTY | O_NDELAY);
	serialport->fd = open("/dev/ttyACM0",  O_RDWR | O_NONBLOCK | O_NOCTTY | O_NDELAY);
	
	ioctl(serialport->fd, MATRIX_SET_UART_TYPE, &serialport->mode); 
	
	tcgetattr(serialport->fd,&term);
	term.c_cflag = (serialport->baud | CS8 | CREAD | CLOCAL | HUPCL);
	term.c_cflag &= ~PARENB;
	//term.c_oflag = 0;//no remapping NO delay
	term.c_oflag &= ~OPOST;//no remapping NO delay
	term.c_iflag = 0;//NO flowcontrol XON XOFF...
	term.c_lflag = 0;//no echo
	tcsetattr(serialport->fd,TCSANOW,&term);
g_print ("Port open: baud=%i\n",serialport->baud);

}

void on_btn_send_telegramm_clicked(GtkButton *button)
{
gchar lc_telegramm[100];

sprintf(lc_telegramm, "%s\n",(gtk_entry_get_text(widgets->w_ntr_telegramm)));

//write(serialport->fd,test,sizeof(test));
write(serialport->fd,lc_telegramm,sizeof(lc_telegramm));
g_print("on_btn_send_telegramm_clicked::\n");
}

//GTK_STATE_NORMAL 	|| GTK_STATE_ACTIVE || GTK_STATE_PRELIGHT
//GTK_STATE_SELECTED	|| GTK_STATE_INSENSITIVE
// gboolean gtk_widget_get_sensitive (GtkWidget *widget);
//gtk_widget_set_sensitive(app_wdgts->w_btn_TossTheCoin, TRUE);
//voi gtk_toggle_button_set_active (GtkToggleButton *toggle_button, TRUE/FALSE);	
void on_btn_rs232_connect_clicked(GtkToggleButton *button)
{

    //if (gtk_widget_get_sensitive (widgets->w_btn_rs232_connect))
    if (!serialport->ser_is_open)
    {
		g_timeout_add(500,(GSourceFunc)recv_data,NULL); //1000 = 1 sec

        gtk_button_set_label(GTK_BUTTON(button), "SERIEL CLOSE");
        serialport->ser_is_open=TRUE;
		open_port();
		gtk_widget_set_sensitive(widgets->w_btn_send_telegramm, TRUE);
	g_print("on_btn_rs232_connect_clicked::opened\n");
    }
    else 
    {
        gtk_button_set_label(GTK_BUTTON(button), "SERIEL CONNECT");
		close(serialport->fd);
		serialport->fd = 0;
		serialport->ser_is_open=FALSE;
		gtk_widget_set_sensitive(widgets->w_btn_send_telegramm, FALSE);
	g_print("on_btn_rs232_connect_clicked::closed\n");
    }

}

void on_rx_tree_view_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column, gpointer user_data)
{
//ForeachFunc
//gtk_tree_model_foreach(GTK_TREE_MODEL(widgets->w_rx_tree_store),ForeachFunc, NULL);
g_print("on_rx_tree_view_row_activated::%s\n","user_data");
}

//gtk_tree_selection_set_mode(SELECTION, TYPE)
//TYPE:
//GTK_SELECTION_NONE
//GTK_SELECTION_SINGLE
//GTK_SELECTION_BROWSE
//GTK_SELECTION_MULTIPLE
void on_rx_tree_select_changed(GtkListBox *box, gpointer data)
{
GtkTreeIter iter;
GtkTreeModel *model;
gchar *name;
GtkTreeSelection *tree_selection;

tree_selection= gtk_tree_view_get_selection (widgets->w_rx_tree_view);

if(gtk_tree_selection_get_selected(tree_selection, &model, &iter))
{
	gtk_tree_model_get (model, &iter, 0, &name, -1);
	g_print("on_rx_tree_select_changed:: name=%s\n", name);
	g_free(name);
}

}

void on_btn_del_selected_clicked(GtkButton *button)
{
//gboolean gtk_list_store_remove (GtkListStore *list_store, GtkTreeIter *iter);

GtkTreeIter iter;
GtkTreeModel *model;
//gchar *name;
GtkTreeSelection *tree_selection;
//char *intvalueOF;

tree_selection= gtk_tree_view_get_selection (widgets->w_rx_tree_view);

if(gtk_tree_selection_get_selected(tree_selection, &model, &iter))
{
	//intvalueOF=(char *)iter;
	g_print("on_btn_del_selected_clicked::iter=%s\n","iter.user_data3");
	gtk_list_store_remove (widgets->w_rx_tree_store, &iter);
	//intvalueOF=(char *)iter;
	g_print("on_btn_del_selected_clicked::iter=%s\n","iter.user_data3");
	//gtk_tree_model_get (model, &iter, 0, &name, -1);
	//g_print("on_rx_tree_select_changed:: name=%s\n", name);
	//g_free(name);
}
}

//on_rx_tree_view_cursor_changed
//gboolean gtk_list_store_remove (GtkListStore *list_store, GtkTreeIter *iter);

void on_btn_init_listview_clicked(GtkButton *button)
{
	GtkTreeIter iter;
	char buf[1024] = "so das hat geklappt\n Ha Ha Ha!\n";

	
	g_print("on_btn_init_listview_clicked::\n");

	gtk_list_store_append (GTK_LIST_STORE (widgets->w_rx_tree_store), &iter);
	gtk_list_store_set (GTK_LIST_STORE(widgets->w_rx_tree_store), &iter,0, buf,-1);

}

GtkWidget *make_menu_item (gchar *name,  GCallback callback, gpointer data)
{
	GtkWidget *item;
	item = gtk_menu_item_new_with_label (name);
	g_signal_connect (item, "activate", callback, data);
	gtk_widget_show (item);
	return item;
}

static void on_changed_port(GtkComboBox *widget, gpointer   user_data)
{
gchar *distro;

g_print ("on_changed_port::\n");

  if (gtk_combo_box_get_active (widgets->w_cbx_port_select) != 0)
  {
    distro = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(widgets->w_cbx_port_select));
    g_print ("You chose Port=%s\n", distro);
    g_free (distro);
  }
}


static void on_changed_baudrate(GtkComboBox *widget, gpointer   user_data)
{
//GtkComboBox *combo_box = widget;
gchar *distro;

g_print ("on_changed_baudrate::\n");

  if (gtk_combo_box_get_active (widgets->w_cbx_baud_select) != 0)
  {
    distro = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(widgets->w_cbx_baud_select));
    g_print ("You chose Baud=%s\n", distro);
    g_free (distro);
  }
}

static void on_changed_mode(GtkComboBox *widget, gpointer   user_data)
{
gchar *distro;

g_print ("on_changed_mode::\n");

  if (gtk_combo_box_get_active (widgets->w_cbx_mode_select) != 0)
  {
    distro = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(widgets->w_cbx_mode_select));
    g_print ("You chose Mode=%s\n", distro);
    g_free (distro);
  }
}



char init_comboboxes()
{

	g_print("init_comboboxes:: \n");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_port_select), "Select a Port");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_port_select), "Port 1");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_port_select), "Port 2");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_port_select), "Port 3");
	gtk_combo_box_set_active (GTK_COMBO_BOX (widgets->w_cbx_port_select), 1);
	g_signal_connect (widgets->w_cbx_port_select, "changed", G_CALLBACK (on_changed_port), NULL);
//--
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_baud_select), "Select-Baud-Rate");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_baud_select), "9600");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_baud_select), "19200");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_baud_select), "38400");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_baud_select), "115200");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_baud_select), "230400");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_baud_select), "460800");
	gtk_combo_box_set_active (GTK_COMBO_BOX (widgets->w_cbx_baud_select), 2);
	g_signal_connect (widgets->w_cbx_baud_select, "changed", G_CALLBACK (on_changed_baudrate), NULL);
//--	
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_mode_select), "Select-Mode");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_mode_select), "RS232");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_mode_select), "RS422");
  	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widgets->w_cbx_mode_select), "RS485");
	gtk_combo_box_set_active (GTK_COMBO_BOX (widgets->w_cbx_mode_select), 1);
	g_signal_connect (widgets->w_cbx_mode_select, "changed", G_CALLBACK (on_changed_mode), NULL);
//--

return TRUE;
}

int main(int argc, char *argv[])
{
    GtkBuilder      *builder; 
    GtkWidget       *window;

widgets = g_slice_new(app_widgets);

serialport = g_slice_new(struc_serialport);
serialport->ser_is_open=FALSE;
serialport->port=1;
serialport->mode=232;
serialport->baud=B19200;
serialport->fd=0;



    gtk_init(&argc, &argv);

    builder = gtk_builder_new_from_file("glade_serial.glade");
    window = GTK_WIDGET(gtk_builder_get_object(builder, "xWindow"));
    // Get pointers to widgets
//    widgets->w_txt_view_main = GTK_WIDGET(gtk_builder_get_object(builder, "txt_view_main"));
//    widgets->w_dlg_file_choose = GTK_WIDGET(gtk_builder_get_object(builder, "dlg_file_choose"));
//    widgets->txt_buf_main = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "txt_buf_main"));
    widgets->w_hlp_mnu_About = GTK_WIDGET(gtk_builder_get_object(builder, "hlp_mnu_about"));
    widgets->w_dlg_about = GTK_WIDGET(gtk_builder_get_object(builder, "dlg_about"));

    widgets->w_lbl_time 	= GTK_WIDGET(gtk_builder_get_object(builder, "lbl_time"));

	widgets->w_cbx_port_select = GTK_COMBO_BOX(gtk_builder_get_object(builder, "cbx_port_select"));
	widgets->w_cbx_baud_select = GTK_COMBO_BOX(gtk_builder_get_object(builder, "cbx_baud_select"));
	widgets->w_cbx_mode_select = GTK_COMBO_BOX(gtk_builder_get_object(builder, "cbx_mode_select"));
//
    widgets->w_btn_rs232_connect= GTK_WIDGET(gtk_builder_get_object(builder, "btn_rs232_connect"));
    widgets->w_btn_send_telegramm= GTK_WIDGET(gtk_builder_get_object(builder, "btn_send_telegramm"));
//
    widgets->w_ntr_telegramm= GTK_ENTRY(gtk_builder_get_object(builder, "ntr_telegramm"));
//
	widgets->w_rx_tree_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "rx_tree_store"));
	//GTK_LIST_STORE
	//widgets->w_rx_tree_view = GTK_LIST_BOX(gtk_builder_get_object(builder, "rx_tree_view"));
	widgets->w_rx_tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "rx_tree_view"));
	//widgets->w_rx_tree_select= GTK_WIDGET(gtk_builder_get_object(builder, "rx_tree_select"));


init_comboboxes();
//
	g_timeout_add(1, (GSourceFunc)timer_handler, widgets);
//
    
    gtk_builder_connect_signals(builder, widgets);

    g_object_unref(builder);

    gtk_widget_show(window);
    gtk_main();
    g_slice_free(app_widgets, widgets);

    return 0;
}

// File --> Open
void on_file_mnu_open_activate(GtkMenuItem *menuitem, app_widgets *app_wdgts)
{
    gchar *file_name = NULL;        // Name of file to open from dialog box
    gchar *file_contents = NULL;    // For reading contents of file
    gboolean file_success = FALSE;  // File read status
    
    // Show the "Open Text File" dialog box
    gtk_widget_show(app_wdgts->w_dlg_file_choose);
    
    // Check return value from Open Text File dialog box to see if user clicked the Open button
    if (gtk_dialog_run(GTK_DIALOG (app_wdgts->w_dlg_file_choose)) == GTK_RESPONSE_OK) {
        // Get the file name from the dialog box
        file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(app_wdgts->w_dlg_file_choose));
        if (file_name != NULL) {
            // Copy the contents of the file to dynamically allocated memory
            file_success = g_file_get_contents(file_name, &file_contents, NULL, NULL);
            if (file_success) {
                // Put the contents of the file into the GtkTextBuffer
                gtk_text_buffer_set_text(app_wdgts->txt_buf_main, file_contents, -1);
            }
            g_free(file_contents);
        }
        g_free(file_name);
    }

    // Finished with the "Open Text File" dialog box, so hide it
    gtk_widget_hide(app_wdgts->w_dlg_file_choose);
}


void on_hlp_mnu_about_activate(GtkMenuItem *menuitem, app_widgets *app_wdgts)
{
    gtk_widget_show(app_wdgts->w_dlg_about);
}

// About dialog box Close button
//   on_dlg_about_response
void on_dlg_about_response(GtkDialog *dialog, gint response_id, app_widgets *app_wdgts)
{
    gtk_widget_hide(app_wdgts->w_dlg_about);
}

// File --> Close
void on_file_mnu_close_activate(GtkMenuItem *menuitem, app_widgets *app_wdgts)
{
    // Clear the text from window "Close the file"
    gtk_text_buffer_set_text(app_wdgts->txt_buf_main, "", -1);
}

// File --> Quit
void on_file_mnu_quit_activate(GtkMenuItem *menuitem, app_widgets *app_wdgts)
{
    //gtk_main_quit();
    on_xWindow_destroy();
}

//-----------------------------------------
gboolean timer_handler(/*app_widgets *app_wdgts*/)
{
    GDateTime   *time;            // for storing current time and date
    gchar       *time_str;        // current time and date as a string

char *format = "<span background=\"#AAFFAA\">%s</span>";
char *markup;

    time = g_date_time_new_now_local();
    time_str = g_date_time_format(time, "%H:%M:%S");
    //"<span foreground='red' weight='bold' font='14'>You Lose!</span>"

	markup = g_markup_printf_escaped (format, time_str);
	gtk_label_set_markup(GTK_LABEL(widgets->w_lbl_time), markup);

//    gtk_label_set_markup(GTK_LABEL (app_wdgts->w_lbl_time), "<span background=\'#00FF00\'></span>");
//    gtk_label_set_markup(GTK_LABEL (widgets->w_lbl_time), "<span foreground='#00FF00'></span>");
//    gtk_label_set_text(GTK_LABEL(app_wdgts->w_lbl_time), time_str);

	free (markup);
    
    g_free (time_str);
	g_date_time_unref(time);
	
    return TRUE;
}

// called when window is closed
void on_xWindow_destroy()
{
    gtk_main_quit();
}
