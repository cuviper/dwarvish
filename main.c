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

#include "session.h"
#include "attrtree.h"
#include "dietree.h"
#include "loaddwfl.h"


/* Like the gtk_builder_new_from_resource in 3.10, but this project's
 * GDK_VERSION_MAX_ALLOWED is not that high yet.  */
static GtkBuilder *
load_gtk_builder (const gchar *resource_path)
{
  GError *error = NULL;
  GtkBuilder *builder = gtk_builder_new ();
  if (!gtk_builder_add_from_resource (builder, resource_path, &error))
    g_error ("ERROR: %s\n", error->message);
  return builder;
}


static GtkWidget *
create_die_widget (DwarvishSession *session, gboolean types)
{
  GtkBuilder *builder = load_gtk_builder ("/dwarvish/die.ui");

  GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (builder, "widget"));
  GtkTreeView *dieview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "dietreeview"));
  GtkTreeView *attrview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "attrtreeview"));

  if (die_tree_view_render (dieview, session, types)
      && attr_tree_view_render (attrview))
    {
      g_object_ref (widget);
      gtk_builder_connect_signals (builder, NULL);
    }
  else
    widget = NULL;

  g_object_unref (builder);
  return widget;
}


static GtkWidget *
create_main_window (DwarvishSession *session)
{
  GtkBuilder *builder = load_gtk_builder ("/dwarvish/application.ui");

  GtkWidget *window = GTK_WIDGET (gtk_builder_get_object (builder, "window"));
  GtkNotebook *notebook = GTK_NOTEBOOK (gtk_builder_get_object (builder, "notebook"));
  GtkLabel *mainfile = GTK_LABEL (gtk_builder_get_object (builder, "mainfile"));
  GtkLabel *debugfile = GTK_LABEL (gtk_builder_get_object (builder, "debugfile"));

  /* Update the title with our module.  */
  gchar *title = g_strdup_printf ("%s - %s", session->basename, PACKAGE_NAME);
  gtk_window_set_title (GTK_WINDOW (window), title);
  g_free (title);

  /* Update the file path labels.  */
  gtk_label_set_text (mainfile, session->mainfile);
  gtk_label_set_text (debugfile, session->debugfile);

  /* Attach the .debug_info view.  */
  GtkWidget *die_widget = create_die_widget (session, FALSE);
  if (die_widget)
    {
      gtk_notebook_append_page (notebook, die_widget, gtk_label_new ("Info"));
      g_object_unref (die_widget);
    }

  /* Attach the .debug_types view.  */
  die_widget = create_die_widget (session, TRUE);
  if (die_widget)
    {
      gtk_notebook_append_page (notebook, die_widget, gtk_label_new ("Types"));
      g_object_unref (die_widget);
    }

  gtk_builder_connect_signals (builder, NULL);
  g_object_unref (builder);
  return window;
}


static void G_GNUC_NORETURN
exit_message (const char *message, gboolean usage)
{
  if (message)
    g_printerr ("%s: %s\n", g_get_application_name (), message);
  if (usage)
    g_printerr ("Try '--help' for more information.\n");
  exit (EXIT_FAILURE);
}


static DwarvishSession *
session_begin (void)
{
  return g_malloc0 (sizeof (DwarvishSession));
}


static void
session_init_dwarf (DwarvishSession *session)
{
  session->dwfl = session->file ? load_elf_dwfl (session->file)
    : load_kernel_dwfl (session->kernel, session->module);
  session->dwflmod = get_first_module (session->dwfl);

  if (session->dwflmod == NULL)
    exit_message ("Couldn't load the requested target.", FALSE);

  Dwarf_Addr bias;
  session->dwarf = dwfl_module_getdwarf (session->dwflmod, &bias);
  if (session->dwarf == NULL)
    exit_message ("No DWARF found for the target.", FALSE);

  const char *modname = dwfl_module_info (session->dwflmod,
                                          NULL, NULL, NULL, NULL, NULL,
                                          &session->mainfile,
                                          &session->debugfile);
  session->basename = g_path_get_basename (modname);
}


static void
session_end (DwarvishSession *session)
{
  g_free (session->kernel);
  g_free (session->module);
  g_free (session->file);

  dwfl_end (session->dwfl);

  g_free (session->basename);

  g_free (session);
}


static gboolean G_GNUC_NORETURN
option_version (G_GNUC_UNUSED const gchar *option_name,
                G_GNUC_UNUSED const gchar *value,
                G_GNUC_UNUSED gpointer data,
                G_GNUC_UNUSED GError **err)
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
  DwarvishSession *session = session_begin ();

  gchar **files = NULL;
  GError *error = NULL;

  GOptionEntry options[] =
    {
        {
          "version", 'V', G_OPTION_FLAG_NO_ARG,
          G_OPTION_ARG_CALLBACK, option_version,
          "Show version information and exit", NULL
        },
        {
          "explicit-imports", 0, 0, G_OPTION_ARG_NONE,
          &session->explicit_imports,
          "Show explicit partial/imported_unit DIEs", NULL
        },
        {
          "kernel", 'k', 0, G_OPTION_ARG_FILENAME, &session->kernel,
          "Load the given kernel release", "RELEASE"
        },
        {
          "module", 'm', 0, G_OPTION_ARG_FILENAME, &session->module,
          "Load the given kernel module name", "MODULE"
        },
        {
          G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &files,
          "Load the given ELF file", "FILE"
        },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

  if (!gtk_init_with_args (&argc, &argv,
                           "| [--kernel=RELEASE] [--module=MODULE]",
                           options, NULL, &error))
    exit_message (error ? error->message : NULL, TRUE);

  if (files)
    {
      if (files[1] || session->kernel || session->module)
        exit_message ("Only one target is supported at a time.", TRUE);
      session->file = g_strdup (files[0]);
      g_strfreev (files);
    }

  session_init_dwarf (session);

  GtkWidget *window = create_main_window (session);
  gtk_widget_show_all (window);
  gtk_main ();

  session_end (session);

  return EXIT_SUCCESS;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
