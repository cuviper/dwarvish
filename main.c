/*
 * dwarvish main code.
 * Copyright (C) 2013  Josh Stone
 *
 * This file is part of dwarvish, and is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <elfutils/libdwfl.h>
#include <gtk/gtk.h>

#include "dietree.h"


static void
cell_guint64_to_hex (GtkTreeViewColumn *tree_column __attribute__ ((unused)),
                     GtkCellRenderer *cell, GtkTreeModel *tree_model,
                     GtkTreeIter *iter, gpointer data)
{
  guint64 num;
  gint index = (gintptr)data;
  gtk_tree_model_get (tree_model, iter, index, &num, -1);

  gchar *text = g_strdup_printf ("%" G_GINT64_MODIFIER "x", num);
  g_object_set (cell, "text", text, NULL);
  g_free (text);
}


static void
add_column (GtkWidget *view, const gchar* title,
            gint column, gboolean hex)
{
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "font", "monospace 9", NULL);

  GtkTreeViewColumn *col = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_title (col, title);

  if (hex)
    gtk_tree_view_column_set_cell_data_func (col, renderer,
                                             cell_guint64_to_hex,
                                             (gpointer)(gintptr)column,
                                             NULL);
  else
    gtk_tree_view_column_add_attribute (col, renderer,
                                        "text", column);

  gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);
}


static GtkWidget *
create_die_tree_view (Dwarf *dwarf, gboolean types)
{
  GtkTreeModel *model = die_tree_new (dwarf, types);

  /* If the die tree is empty, just say so.  */
  GtkTreeIter iter;
  if (model == NULL || !gtk_tree_model_get_iter_first (model, &iter))
    {
      if (model != NULL)
        g_object_unref (model);
      return gtk_label_new ("no data found");
    }

  GtkWidget *scrollwin = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *view = gtk_tree_view_new_with_model (model);
  g_object_unref (model); /* the view keeps its own reference */

  add_column (view, "Offset", DIE_TREE_COL_OFFSET, TRUE);
  add_column (view, "Tag", DIE_TREE_COL_TAG_STRING, FALSE);
  add_column (view, "Name", DIE_TREE_COL_NAME, FALSE);

  gtk_container_add (GTK_CONTAINER (scrollwin), view);
  return scrollwin;
}


static Dwfl *
load_kernel (void)
{
  static const Dwfl_Callbacks kernel_callbacks =
    {
      dwfl_linux_kernel_find_elf,
      dwfl_standard_find_debuginfo,
      dwfl_linux_kernel_module_section_address,
      NULL
    };

  Dwfl *dwfl = dwfl_begin (&kernel_callbacks);
  dwfl_report_begin (dwfl);
  dwfl_linux_kernel_report_kernel (dwfl);

  return dwfl;
}


static int
get_first_dwarf_cb (Dwfl_Module *mod __attribute__ ((unused)),
                    void **userdata __attribute__ ((unused)),
                    const char *name __attribute__ ((unused)),
                    Dwarf_Addr start __attribute__ ((unused)),
                    Dwarf *dwarf,
                    Dwarf_Addr bias __attribute__ ((unused)),
                    void *arg)
{
  *(Dwarf**)arg = dwarf;
  return DWARF_CB_ABORT;
}


static Dwarf *
get_first_dwarf (Dwfl *dwfl)
{
  Dwarf *dwarf = NULL;
  dwfl_getdwarf (dwfl, get_first_dwarf_cb, &dwarf, 0);
  return dwarf;
}


int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 500, 500);
  g_signal_connect (window, "delete_event", gtk_main_quit, NULL);

  GtkWidget *notebook = gtk_notebook_new ();

  /* For now, just load debuginfo for the running kernel.  */
  Dwfl *dwfl = load_kernel ();
  Dwarf *dwarf = get_first_dwarf (dwfl);

  GtkWidget *scrollwin = create_die_tree_view (dwarf, FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrollwin,
                            gtk_label_new ("Info"));

  scrollwin = create_die_tree_view (dwarf, TRUE);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrollwin,
                            gtk_label_new ("Types"));

  gtk_container_add (GTK_CONTAINER (window), notebook);

  gtk_widget_show_all (window);

  gtk_main ();

  dwfl_end (dwfl);

  return 0;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
