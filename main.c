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

#include <gtk/gtk.h>

#include "attrtree.h"
#include "dietree.h"
#include "loaddwfl.h"


#ifndef GDK_VERSION_3_10
GtkBuilder *
gtk_builder_new_from_resource (const gchar *resource_path)
{
  GError *error = NULL;
  GtkBuilder *builder = gtk_builder_new ();
  if (!gtk_builder_add_from_resource (builder, resource_path, &error))
    g_error ("ERROR: %s\n", error->message);
  return builder;
}
#endif


static GtkWidget *
create_die_widget (Dwarf *dwarf, gboolean types)
{
  GtkBuilder *builder = gtk_builder_new_from_resource ("/dwarvish/die.ui");
  gtk_builder_connect_signals (builder, NULL);

  GtkWidget *window = GTK_WIDGET (gtk_builder_get_object (builder, "window"));
  GtkTreeView *dieview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "dietreeview"));
  GtkTreeView *attrview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "attrtreeview"));
  GtkLabel *attrlabel = GTK_LABEL (gtk_builder_get_object (builder, "attrlabel"));

  g_object_ref (window);
  g_object_unref (builder);

  die_tree_view_render (dieview, dwarf, types);
  attr_tree_view_render (dieview, attrview, attrlabel, dwarf, types);
  return window;
}


static GtkWidget *
create_main_window (Dwarf *dwarf, const char *modname)
{
  GtkBuilder *builder = gtk_builder_new_from_resource ("/dwarvish/application.ui");
  gtk_builder_connect_signals (builder, NULL);

  GtkWidget *window = GTK_WIDGET (gtk_builder_get_object (builder, "window"));
  GtkWidget *notebook = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));
  g_object_unref (builder);

  /* Update the title with our module.  */
  gchar *title = g_strdup_printf ("%s - %s", modname, PACKAGE_NAME);
  gtk_window_set_title (GTK_WINDOW (window), title);
  g_free (title);

  GtkWidget *die_window = create_die_widget (dwarf, FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), die_window,
                            gtk_label_new ("Info"));
  g_object_unref (die_window);

  die_window = create_die_widget (dwarf, TRUE);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), die_window,
                            gtk_label_new ("Types"));
  g_object_unref (die_window);

  return window;
}


static void __attribute__ ((noreturn))
exit_message (const char *message, gboolean usage)
{
  if (message)
    g_printerr ("%s: %s\n", g_get_application_name (), message);
  if (usage)
    g_printerr ("Try '--help' for more information.\n");
  exit (EXIT_FAILURE);
}


gboolean __attribute__ ((noreturn))
option_version (const gchar *option_name __attribute__ ((unused)),
                const gchar *value __attribute__ ((unused)),
                gpointer data __attribute__ ((unused)),
                GError **err __attribute__ ((unused)))
{
  g_print ("%s\n", PACKAGE_STRING);
  g_print ("Copyright (C) 2013  Josh Stone\n");
  g_print ("License GPLv3+: GNU GPL version 3 or later"
           " <http://gnu.org/licenses/gpl.html>.\n");
  g_print ("This is free software:"
           " you are free to change and redistribute it.\n");
  g_print ("There is NO WARRANTY, to the extent permitted by law.\n");
  exit (EXIT_SUCCESS);
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
          "version", 'V', G_OPTION_FLAG_NO_ARG,
          G_OPTION_ARG_CALLBACK, option_version,
          "Show version information and exit", NULL
        },
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
                           "| [--kernel=RELEASE] [--module=MODULE]",
                           options, NULL, &error))
    exit_message (error ? error->message : NULL, TRUE);

  if (files && (files[1] || kernel || module))
    exit_message ("Only one target is supported at a time.", TRUE);

  Dwfl *dwfl = files ? load_elf_dwfl (files[0])
    : load_kernel_dwfl (kernel, module);
  if (dwfl == NULL)
    exit_message ("Couldn't load the requested target.", FALSE);

  Dwfl_Module *mod = get_first_module (dwfl);
  const char *modname = dwfl_module_info (mod, NULL, NULL, NULL,
                                          NULL, NULL, NULL, NULL);

  Dwarf_Addr bias;
  Dwarf *dwarf = dwfl_module_getdwarf (mod, &bias);
  if (dwarf == NULL)
    exit_message ("No DWARF found for the target.", FALSE);

  GtkWidget *window = create_main_window (dwarf, modname);
  gtk_widget_show_all (window);
  gtk_main ();

  dwfl_end (dwfl);

  return EXIT_SUCCESS;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
