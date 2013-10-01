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

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "dietree.h"
#include "loaddwfl.h"


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


static GtkWidget *
create_main_window (Dwarf *dwarf)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 500, 500);
  g_signal_connect (window, "delete_event", gtk_main_quit, NULL);

  GtkWidget *notebook = gtk_notebook_new ();

  GtkWidget *scrollwin = create_die_tree_view (dwarf, FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrollwin,
                            gtk_label_new ("Info"));

  scrollwin = create_die_tree_view (dwarf, TRUE);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrollwin,
                            gtk_label_new ("Types"));

  gtk_container_add (GTK_CONTAINER (window), notebook);
  return window;
}


static void __attribute__ ((noreturn))
exit_message (const char *message, gboolean usage)
{
  if (message)
    fprintf (stderr, "%s: %s\n", g_get_application_name (), message);
  if (usage)
    fprintf (stderr, "Try '--help' for more information.\n");
  exit (EXIT_FAILURE);
}


int
main (int argc, char **argv)
{
  gchar* kernel = NULL;
  gchar* module = NULL;
  gchar** files = NULL;
  GError *error = NULL;

  GOptionEntry options[] =
    {
        {
          "kernel", 'k', 0, G_OPTION_ARG_FILENAME, &kernel,
          "Load the given kernel release.", "RELEASE"
        },
        {
          "module", 'm', 0, G_OPTION_ARG_FILENAME, &module,
          "Load the given kernel module name.", "MODULE"
        },
        {
          G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &files,
          "Load the given ELF file.", "FILE"
        },
        { NULL }
    };

  if (!gtk_init_with_args (&argc, &argv,
                           "| [--kernel=KERNEL] [--module=MODULE]",
                           options, NULL, &error))
    exit_message (error ? error->message : NULL, TRUE);

  if (files && (files[1] || kernel || module))
    exit_message ("Only one target is supported at a time.", TRUE);

  Dwfl *dwfl = files ? load_elf_dwfl (files[0])
    : load_kernel_dwfl (kernel, module);
  if (dwfl == NULL)
    exit_message ("Couldn't load the requested target.", FALSE);

  Dwarf *dwarf = get_first_dwarf (dwfl);
  if (dwarf == NULL)
    exit_message ("No DWARF found in the target.", FALSE);

  GtkWidget *window = create_main_window (dwarf);
  gtk_widget_show_all (window);
  gtk_main ();

  dwfl_end (dwfl);

  return EXIT_SUCCESS;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
