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
#include "known-dwarf.h"

#include "dietree.h"


typedef struct _DieTreeData
{
  Dwarf *dwarf;
  gboolean types;
} DieTreeData;


static const char *
dwarf_tag_string (unsigned int tag)
{
  switch (tag)
    {
#define ONE_KNOWN_DW_TAG(NAME, CODE) case CODE: return #NAME;
      ALL_KNOWN_DW_TAG
#undef ONE_KNOWN_DW_TAG
    default:
      return NULL;
    }
}


static Dwarf_Off
die_tree_iter_offset (GtkTreeModel *model, GtkTreeIter *iter)
{
  guint64 offset;
  gtk_tree_model_get (model, iter, DIE_TREE_COL_OFFSET, &offset, -1);
  return offset;
}


static Dwarf_Off
die_tree_offdie (Dwarf *dwarf, gboolean types,
                 Dwarf_Off offset, Dwarf_Die *die)
{
  return (types ? dwarf_offdie_types (dwarf, offset, die)
          : dwarf_offdie (dwarf, offset, die)) != NULL;
}


static void
die_tree_set_values (GtkTreeStore *store, GtkTreeIter *iter, Dwarf_Die *die)
{
  Dwarf_Off off = dwarf_dieoffset (die);
  int tag = dwarf_tag (die);
  const char *tagstr = dwarf_tag_string (tag);
  const char *name = dwarf_diename (die);
  gtk_tree_store_set (store, iter,
                      DIE_TREE_COL_OFFSET, off,
                      DIE_TREE_COL_NAME, name,
                      DIE_TREE_COL_TAG, tag,
                      DIE_TREE_COL_TAG_STRING, tagstr,
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
  if (!die_tree_offdie (data->dwarf, data->types,
                        die_tree_iter_offset (model, iter), &die))
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


GtkWidget *
die_tree_view_new (Dwarf *dwarf, gboolean types)
{
  gboolean empty = TRUE;
  GtkTreeStore *store = gtk_tree_store_new (DIE_TREE_N_COLUMNS,
                                            G_TYPE_UINT64,
                                            G_TYPE_STRING,
                                            G_TYPE_INT,
                                            G_TYPE_STRING
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

  if (empty)
    {
      g_object_unref (store);
      return NULL;
    }

  GtkWidget *view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store); /* The view keeps its own reference.  */

  DieTreeData *data = g_malloc (sizeof (DieTreeData));
  data->dwarf = dwarf;
  data->types = types;
  g_signal_connect_swapped (view, "destroy", G_CALLBACK (g_free), data);
  g_signal_connect (view, "row-expanded",
                    G_CALLBACK (die_tree_expand_row), data);

  return view;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
