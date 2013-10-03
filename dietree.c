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


/* The data columns stored in the tree model.  */
enum
{
  DIE_TREE_COL_OFFSET = 0,
  DIE_TREE_COL_TAG,
  DIE_TREE_COL_NAME,
  DIE_TREE_N_COLUMNS,
};


typedef struct _DieTreeData
{
  Dwarf *dwarf;
  gboolean types;
} DieTreeData;


static Dwarf_Off
die_tree_iter_offset (GtkTreeModel *model, GtkTreeIter *iter)
{
  guint64 offset;
  gtk_tree_model_get (model, iter, DIE_TREE_COL_OFFSET, &offset, -1);
  return offset;
}


static gboolean
die_tree_offdie (Dwarf *dwarf, gboolean types,
                 Dwarf_Off offset, Dwarf_Die *die)
{
  return (types ? dwarf_offdie_types (dwarf, offset, die)
          : dwarf_offdie (dwarf, offset, die)) != NULL;
}


gboolean
die_tree_get_die (GtkTreeModel *model, GtkTreeIter *iter,
                  Dwarf *dwarf, gboolean types, Dwarf_Die *die)
{
  Dwarf_Off offset = die_tree_iter_offset (model, iter);
  return die_tree_offdie (dwarf, types, offset, die);
}


static void
die_tree_set_values (GtkTreeStore *store, GtkTreeIter *iter, Dwarf_Die *die)
{
  Dwarf_Off off = dwarf_dieoffset (die);
  const char *tag = DW_TAG__string (dwarf_tag (die));
  const char *name = dwarf_diename (die);
  gtk_tree_store_set (store, iter,
                      DIE_TREE_COL_OFFSET, off,
                      DIE_TREE_COL_TAG, tag,
                      DIE_TREE_COL_NAME, name,
                      -1);

  if (dwarf_haschildren (die))
    {
      GtkTreeIter placeholder;
      gtk_tree_store_insert_with_values (store, &placeholder, iter, 0,
                                         DIE_TREE_COL_OFFSET, G_MAXUINT64,
                                         -1);
    }
}


static void
die_tree_expand_row (GtkTreeView *tree_view,
                     GtkTreeIter *iter,
                     GtkTreePath *path __attribute__ ((unused)),
                     gpointer user_data)
{
  DieTreeData *data = (DieTreeData *)user_data;
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);

  GtkTreeIter child;
  if (!gtk_tree_model_iter_children (model, &child, iter))
    g_return_if_reached(); /* Should always have a child when expanding!  */

  GtkTreeIter next = child;
  if (gtk_tree_model_iter_next (model, &next))
    return; /* Already have siblings, so we're done.  */

  if (die_tree_iter_offset (model, &child) != G_MAXUINT64)
    return; /* It's an only child, but not the placeholder.  */

  Dwarf_Die die;
  if (!die_tree_get_die (model, iter, data->dwarf, data->types, &die))
    g_return_if_reached(); /* The parent offset doesn't translate!  */

  /* Fill in the real children.  */
  GtkTreeStore *store = GTK_TREE_STORE (model);
  if (dwarf_child (&die, &die) == 0)
    {
      die_tree_set_values (store, &child, &die);
      while (dwarf_siblingof (&die, &die) == 0)
        {
          gtk_tree_store_insert_after (store, &child, iter, &child);
          die_tree_set_values (store, &child, &die);
        }
    }
}


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
die_tree_render_columns (GtkTreeView *view)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "font", "monospace 9", NULL);

  col = gtk_tree_view_get_column (view, 0);
  gpointer ptr = (gpointer)(gintptr)DIE_TREE_COL_OFFSET;
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (col, renderer,
                                           cell_guint64_to_hex,
                                           ptr, NULL);

  col = gtk_tree_view_get_column (view, 1);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text",
                                      DIE_TREE_COL_TAG);

  col = gtk_tree_view_get_column (view, 2);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text",
                                      DIE_TREE_COL_NAME);
}


gboolean
die_tree_view_render (GtkTreeView *view, Dwarf *dwarf, gboolean types)
{
  gboolean empty = TRUE;
  GtkTreeStore *store = gtk_tree_store_new (DIE_TREE_N_COLUMNS,
                                            G_TYPE_UINT64, /* OFFSET */
                                            G_TYPE_STRING, /* TAG */
                                            G_TYPE_STRING  /* NAME */
                                            );

  size_t cuhl;
  uint64_t type_signature;
  Dwarf_Off noff, off = 0;
  GtkTreeIter iter, *sibling = NULL;
  while (dwarf_next_unit (dwarf, off, &noff, &cuhl, NULL, NULL, NULL, NULL,
                          (types ? &type_signature : NULL), NULL) == 0)
    {
      Dwarf_Die die;
      if (!die_tree_offdie (dwarf, types, off + cuhl, &die))
        continue;

      empty = FALSE;
      gtk_tree_store_insert_after (store, &iter, NULL, sibling);
      die_tree_set_values (store, &iter, &die);
      sibling = &iter;
      off = noff;
    }

  gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
  g_object_unref (store); /* The view keeps its own reference.  */

  die_tree_render_columns (view);

  DieTreeData *data = g_malloc (sizeof (DieTreeData));
  data->dwarf = dwarf;
  data->types = types;
  g_signal_connect_swapped (view, "destroy", G_CALLBACK (g_free), data);
  g_signal_connect (view, "row-expanded",
                    G_CALLBACK (die_tree_expand_row), data);

  return !empty;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
