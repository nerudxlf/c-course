#include <gtk/gtk.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

GtkTreeStore *treestore;

void add_directory_to_tree(const char *path, GtkTreeIter *parent) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(fullpath, &st) == -1) continue;

        GtkTreeIter iter;
        if (S_ISDIR(st.st_mode)) {
            gtk_tree_store_append(treestore, &iter, parent);
            gtk_tree_store_set(treestore, &iter, 0, entry->d_name, 1, "📁", -1);
            add_directory_to_tree(fullpath, &iter);
        } else if (S_ISREG(st.st_mode)) {
            gtk_tree_store_append(treestore, &iter, parent);
            gtk_tree_store_set(treestore, &iter, 0, entry->d_name, 1, "📄", -1);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Файлы и папки");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    treestore = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

    add_directory_to_tree(".", NULL);

    GtkWidget *treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(treestore));
    g_object_unref(treestore);

    GtkCellRenderer *renderer_icon = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column_icon = gtk_tree_view_column_new_with_attributes(
        "", renderer_icon, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column_icon);

    GtkCellRenderer *renderer_text = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column_text = gtk_tree_view_column_new_with_attributes(
        "", renderer_text, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column_text);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), treeview);
    gtk_container_add(GTK_CONTAINER(window), scroll);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
