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

#include "dietree.h"
#include "dwstring.h"
#include "util.h"


G_DEFINE_SLICED_COPY_FREE (Dwarf_Die, dwarf_die);
static G_DEFINE_SLICED_BOXED_TYPE (Dwarf_Die, dwarf_die);
#define G_TYPE_DWARF_DIE (dwarf_die_get_type ())


gboolean
die_tree_get_die (GtkTreeModel *model, GtkTreeIter *iter,
                  Dwarf_Die *die_mem)
{
  Dwarf_Die *die = NULL;
  gtk_tree_model_get (model, iter, 0, &die, -1);
  if (die == NULL)
    return FALSE;
  *die_mem = *die;
  dwarf_die_free (die);
  return TRUE;
}


static void
die_tree_set_die (GtkTreeStore *store, GtkTreeIter *iter, Dwarf_Die *die)
{
  gtk_tree_store_set (store, iter, 0, die, -1);
  if (dwarf_haschildren (die))
    {
      GtkTreeIter placeholder;
      gtk_tree_store_prepend (store, &placeholder, iter);
    }
}


G_MODULE_EXPORT void
signal_die_tree_expand_row (GtkTreeView *tree_view, GtkTreeIter *iter,
                            G_GNUC_UNUSED GtkTreePath *path,
                            G_GNUC_UNUSED gpointer user_data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);

  GtkTreeIter child;
  if (!gtk_tree_model_iter_children (model, &child, iter))
    g_return_if_reached(); /* Should always have a child when expanding!  */

  Dwarf_Die die;
  if (die_tree_get_die (model, &child, &die))
    return; /* It's not just a placeholder, we're done.  */

  if (!die_tree_get_die (model, iter, &die))
    g_return_if_reached ();

  /* Fill in the real children.  */
  GtkTreeStore *store = GTK_TREE_STORE (model);
  if (dwarf_child (&die, &die) == 0)
    {
      die_tree_set_die (store, &child, &die);
      while (dwarf_siblingof (&die, &die) == 0)
        {
          gtk_tree_store_insert_after (store, &child, iter, &child);
          die_tree_set_die (store, &child, &die);
        }
    }
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

  gtk_tree_view_set_tooltip_row (view, tooltip, path);
  gtk_tree_path_free (path);

  Dwarf_Die die;
  if (!die_tree_get_die (model, &iter, &die))
    g_return_val_if_reached (FALSE);

  const gchar *name = dwarf_diename (&die);
  gtk_tooltip_set_text (tooltip, name);

  return (name != NULL);
}


enum
{
  DIE_TREE_COL_OFFSET,
  DIE_TREE_COL_TAG,
  DIE_TREE_COL_NAME,
};


static void
die_tree_cell_data (G_GNUC_UNUSED GtkTreeViewColumn *column,
                    GtkCellRenderer *cell, GtkTreeModel *model,
                    GtkTreeIter *iter, gpointer data)
{
  Dwarf_Die die;
  if (!die_tree_get_die (model, iter, &die))
    g_return_if_reached ();

  gchar *alloc = NULL;
  const gchar *fixed = NULL;
  switch (GPOINTER_TO_INT (data))
    {
    case DIE_TREE_COL_OFFSET:
      alloc = g_strdup_printf ("%" G_GINT64_MODIFIER "x",
                               dwarf_dieoffset (&die));
      break;

    case DIE_TREE_COL_TAG:
      fixed = DW_TAG__string (dwarf_tag (&die));
      if (G_UNLIKELY (fixed == NULL))
        alloc = g_strdup_printf ("%#x", dwarf_tag (&die));
      break;

    case DIE_TREE_COL_NAME:
      fixed = dwarf_diename (&die);
      break;
    }

  g_object_set (cell, "text", alloc ?: fixed, NULL);
  if (alloc != NULL)
    g_free (alloc);
}


static void
die_tree_render_column (GtkTreeView *view, gint column, gint virtual_column)
{
  GtkTreeViewColumn *col = gtk_tree_view_get_column (view, column);
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "font", "monospace 9", NULL);
  if (virtual_column == DIE_TREE_COL_NAME)
    g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (col, renderer, die_tree_cell_data,
                                           GINT_TO_POINTER (virtual_column),
                                           NULL);
}


gboolean
die_tree_view_render (GtkTreeView *view, Dwarf *dwarf, gboolean types)
{
  GtkTreeStore *store = gtk_tree_store_new (1, G_TYPE_DWARF_DIE);

  uint64_t type_signature;
  uint64_t *ptype_signature = types ? &type_signature : NULL;
  typeof (dwarf_offdie) *offdie = types ? dwarf_offdie_types : dwarf_offdie;

  gboolean empty = TRUE;
  size_t cuhl;
  Dwarf_Off noff, off = 0;
  GtkTreeIter iter, *sibling = NULL;
  while (dwarf_next_unit (dwarf, off, &noff, &cuhl, NULL, NULL, NULL, NULL,
                          ptype_signature, NULL) == 0)
    {
      Dwarf_Die die;
      if (offdie (dwarf, off + cuhl, &die) == NULL)
        continue;

      empty = FALSE;
      gtk_tree_store_insert_after (store, &iter, NULL, sibling);
      die_tree_set_die (store, &iter, &die);
      sibling = &iter;
      off = noff;
    }

  gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
  g_object_unref (store); /* The view keeps its own reference.  */

  die_tree_render_column (view, 0, DIE_TREE_COL_OFFSET);
  die_tree_render_column (view, 1, DIE_TREE_COL_TAG);
  die_tree_render_column (view, 2, DIE_TREE_COL_NAME);

  return !empty;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
