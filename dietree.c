/*
 * die-tree view implementation.
 * Copyright (C) 2013  Josh Stone
 *
 * This file is part of dwarvish, and is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <dwarf.h>
#include "dietree.h"
#include "dwstring.h"
#include "util.h"


enum
{
  DIE_TREE_COL_OFFSET = 0,
  DIE_TREE_COL_TAG,
  DIE_TREE_COL_NAME,
  DIE_TREE_INT_DIE,
  DIE_TREE_N_COLUMNS
};


static G_DEFINE_SLICED_COPY (Dwarf_Die, dwarf_die);
G_DEFINE_SLICED_FREE (Dwarf_Die, dwarf_die);
G_DEFINE_SLICED_BOXED_TYPE (Dwarf_Die, dwarf_die);


gboolean
die_tree_get_die (GtkTreeModel *model, GtkTreeIter *iter,
                  Dwarf_Die *die_mem)
{
  Dwarf_Die *die = NULL;
  gtk_tree_model_get (model, iter, DIE_TREE_INT_DIE, &die, -1);
  if (die == NULL)
    return FALSE;
  *die_mem = *die;
  dwarf_die_free (die);
  return TRUE;
}


static void
die_tree_set_die (GtkTreeStore *store, GtkTreeIter *iter, Dwarf_Die *die)
{
  gchar *offset = g_strdup_printf ("%" G_GINT64_MODIFIER "x",
                                   dwarf_dieoffset (die));
  gchar *tag = DW_TAG__strdup_hex (dwarf_tag (die));
  const gchar *name = dwarf_diename (die);

  gtk_tree_store_set (store, iter,
                      DIE_TREE_COL_OFFSET, offset,
                      DIE_TREE_COL_TAG, tag,
                      DIE_TREE_COL_NAME, name,
                      DIE_TREE_INT_DIE, die,
                      -1);

  g_free (offset);
  g_free (tag);

  if (dwarf_haschildren (die))
    {
      GtkTreeIter placeholder;
      gtk_tree_store_prepend (store, &placeholder, iter);
    }
}


static GtkTreeIter *
expand_die_children (GtkTreeStore *store, GtkTreeIter *iter,
                     GtkTreeIter *parent, GtkTreeIter *sibling,
                     Dwarf_Die *die, gboolean explicit_imports)
{
  Dwarf_Die child;
  if (dwarf_child (die, &child) == 0)
    do
      {
        if (!explicit_imports &&
            dwarf_tag (&child) == DW_TAG_imported_unit)
          {
            Dwarf_Die import;
            Dwarf_Attribute attr;
            if (dwarf_attr (&child, DW_AT_import, &attr) != NULL &&
                dwarf_formref_die (&attr, &import) != NULL)
              sibling = expand_die_children (store, iter, parent, sibling,
                                             &import, explicit_imports);
            continue;
          }

        if (sibling != NULL)
          gtk_tree_store_insert_after (store, iter, parent, sibling);
        die_tree_set_die (store, iter, &child);
        sibling = iter;
      }
    while (dwarf_siblingof (&child, &child) == 0);
  return sibling;
}


G_MODULE_EXPORT gboolean
signal_die_tree_test_expand_row (GtkTreeView *tree_view, GtkTreeIter *iter,
                                 G_GNUC_UNUSED GtkTreePath *path,
                                 G_GNUC_UNUSED gpointer user_data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);

  GtkTreeIter first_child;
  if (!gtk_tree_model_iter_children (model, &first_child, iter))
    /* Should always have a child when expanding!  */
    g_return_val_if_reached(TRUE);

  Dwarf_Die die;
  if (die_tree_get_die (model, &first_child, &die))
    return FALSE; /* It's not just a placeholder, we're done.  */

  if (!die_tree_get_die (model, iter, &die))
    g_return_val_if_reached(TRUE);

  /* Fill in the real children.  */
  DwarvishSession *session = g_object_get_data (G_OBJECT (model),
                                                "DwarvishSession");
  GtkTreeStore *store = GTK_TREE_STORE (model);
  GtkTreeIter child = first_child;
  GtkTreeIter *last = expand_die_children (store, &child, iter, NULL,
                                           &die, session->explicit_imports);

  if (last != NULL)
    return FALSE;

  /* Nothing was inserted, so remove the placeholder.
   * NB: This leaves no children -- must have been empty imports?!  */
  gtk_tree_store_remove (store, &first_child);
  return TRUE;
}


G_MODULE_EXPORT gboolean
signal_die_tree_query_tooltip (GtkWidget *widget,
                               gint x, gint y, gboolean keyboard_mode,
                               GtkTooltip *tooltip,
                               G_GNUC_UNUSED gpointer user_data)
{
  GtkTreeView *view = GTK_TREE_VIEW (widget);

  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  if (!gtk_tree_view_get_tooltip_context (view, &x, &y, keyboard_mode,
                                          &model, &path, &iter))
    return FALSE;

  gchar *name = NULL;
  gtk_tree_model_get (model, &iter, DIE_TREE_COL_NAME, &name, -1);
  if (name != NULL)
    {
      gtk_tooltip_set_text (tooltip, name);
      gtk_tree_view_set_tooltip_row (view, tooltip, path);
      g_free (name);
    }

  gtk_tree_path_free (path);

  return (name != NULL);
}


static void
die_tree_render_column (GtkTreeView *view, gint column)
{
  GtkTreeViewColumn *col = gtk_tree_view_get_column (view, column);
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "font", "monospace 9", NULL);
  if (column == DIE_TREE_COL_NAME)
    g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text", column);
}


gboolean
die_tree_view_render (GtkTreeView *view, DwarvishSession *session,
                      gboolean types)
{
  GtkTreeStore *store = gtk_tree_store_new (DIE_TREE_N_COLUMNS,
                                            G_TYPE_STRING,
                                            G_TYPE_STRING,
                                            G_TYPE_STRING,
                                            G_TYPE_DWARF_DIE);
  g_object_set_data (G_OBJECT (store), "DwarvishSession", session);

  uint64_t type_signature;
  uint64_t *ptype_signature = types ? &type_signature : NULL;
  typeof (dwarf_offdie) *offdie = types ? dwarf_offdie_types : dwarf_offdie;

  gboolean empty = TRUE;
  size_t cuhl;
  GtkTreeIter iter, *sibling = NULL;
  for (Dwarf_Off noff, off = 0;
       dwarf_next_unit (session->dwarf, off, &noff, &cuhl, NULL,
                        NULL, NULL, NULL, ptype_signature, NULL) == 0;
       off = noff)
    {
      Dwarf_Die die;
      if (offdie (session->dwarf, off + cuhl, &die) == NULL)
        continue;

      if (!session->explicit_imports &&
          dwarf_tag (&die) == DW_TAG_partial_unit)
        continue;

      empty = FALSE;
      gtk_tree_store_insert_after (store, &iter, NULL, sibling);
      die_tree_set_die (store, &iter, &die);
      sibling = &iter;
    }

  gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
  g_object_unref (store); /* The view keeps its own reference.  */

  die_tree_render_column (view, DIE_TREE_COL_OFFSET);
  die_tree_render_column (view, DIE_TREE_COL_TAG);
  die_tree_render_column (view, DIE_TREE_COL_NAME);

  return !empty;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
