/*
 * attr-tree view implementation.
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

#include "attrtree.h"
#include "dietree.h"


/* The data columns stored in the tree model.  */
enum
{
  ATTR_TREE_COL_ATTRIBUTE = 0,
  ATTR_TREE_COL_VALUE,
  ATTR_TREE_N_COLUMNS,
};


typedef struct _AttrTreeData
{
  GtkTreeView *view;
  Dwarf *dwarf;
  gboolean types;
} AttrTreeData;


static const char *
dwarf_attr_string (int code)
{
  switch (code)
    {
#define ONE_KNOWN_DW_AT(NAME, CODE) case CODE: return #NAME;
      ALL_KNOWN_DW_AT
#undef ONE_KNOWN_DW_AT
    default:
      return NULL;
    }
}


typedef struct _AttrCallback
{
  GtkTreeStore *store;
  GtkTreeIter *parent;
  GtkTreeIter *sibling;
  GtkTreeIter iter;
} AttrCallback;


int
getattrs_callback (Dwarf_Attribute *attr, void *user_data)
{
  AttrCallback *data = (AttrCallback *)user_data;

  gtk_tree_store_insert_after (data->store, &data->iter,
                               data->parent, data->sibling);

  int code = dwarf_whatattr (attr);
  const char *string = dwarf_attr_string (code);
  gtk_tree_store_set (data->store, &data->iter,
                      ATTR_TREE_COL_ATTRIBUTE, string,
                      /* TODO ATTR_TREE_COL_VALUE  */
                      -1);

  /* TODO recurse children as dwarf_attr_integrate does.  */

  data->sibling = &data->iter;
  return DWARF_CB_OK;
}


static void
die_tree_selection_changed (GtkTreeSelection *selection,
                            gpointer user_data)
{
  AttrTreeData *data = (AttrTreeData *)user_data;
  GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (data->view));
  gtk_tree_store_clear (store);

  Dwarf_Die die;
  GtkTreeIter iter;
  GtkTreeModel *model;
  if (!gtk_tree_selection_get_selected (selection, &model, &iter) ||
      !die_tree_get_die (model, &iter, data->dwarf, data->types, &die))
    return;

  AttrCallback cbdata;
  cbdata.store = store;
  cbdata.parent = NULL;
  cbdata.sibling = NULL;
  dwarf_getattrs (&die, getattrs_callback, &cbdata, 0);
}


static void
attr_tree_render_columns (GtkTreeView *view)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "font", "monospace 9", NULL);

  col = gtk_tree_view_get_column (view, 0);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text",
                                      ATTR_TREE_COL_ATTRIBUTE);

  col = gtk_tree_view_get_column (view, 1);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text",
                                      ATTR_TREE_COL_VALUE);
}


gboolean
attr_tree_view_render (GtkTreeView *dietree, GtkTreeView *attrtree,
                       Dwarf *dwarf, gboolean types)
{
  GtkTreeStore *store = gtk_tree_store_new (ATTR_TREE_N_COLUMNS,
                                            G_TYPE_STRING, /* ATTRIBUTE */
                                            G_TYPE_STRING  /* VALUE */
                                            );
  gtk_tree_view_set_model (attrtree, GTK_TREE_MODEL (store));
  g_object_unref (store); /* The view keeps its own reference.  */

  attr_tree_render_columns (attrtree);

  AttrTreeData *data = g_malloc (sizeof (AttrTreeData));
  data->view = attrtree;
  data->dwarf = dwarf;
  data->types = types;
  g_signal_connect_swapped (attrtree, "destroy", G_CALLBACK (g_free), data);
  g_signal_connect (gtk_tree_view_get_selection (dietree), "changed",
                    G_CALLBACK (die_tree_selection_changed), data);

  return TRUE;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
