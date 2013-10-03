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
#include "dwstring.h"

#include "attrtree.h"
#include "dietree.h"


/* The data columns stored in the tree model.  */
enum
{
  ATTR_TREE_COL_ATTRIBUTE = 0,
  ATTR_TREE_COL_FORM,
  ATTR_TREE_COL_VALUE,
  ATTR_TREE_N_COLUMNS,
};


typedef struct _AttrTreeData
{
  GtkTreeView *view;
  Dwarf *dwarf;
  gboolean types;
} AttrTreeData;


typedef struct _AttrCallback
{
  GtkTreeStore *store;
  GtkTreeIter *parent;
  GtkTreeIter *sibling;
  GtkTreeIter iter;
} AttrCallback;


const char *
attr_value (Dwarf_Attribute *attr, char **value, Dwarf_Die *ref)
{
  bool flag;
  Dwarf_Addr addr;
  Dwarf_Sword sword;
  Dwarf_Word word;

  switch (dwarf_whatform (attr))
    {
    case DW_FORM_addr:
      if (dwarf_formaddr (attr, &addr) == 0)
        {
          *value = g_strdup_printf ("%#" G_GINT64_MODIFIER "x", addr);
          return *value;
        }
      return NULL;

    case DW_FORM_indirect:
    case DW_FORM_strp:
    case DW_FORM_string:
    case DW_FORM_GNU_strp_alt:
      return dwarf_formstring (attr);

    case DW_FORM_ref_addr:
    case DW_FORM_ref_udata:
    case DW_FORM_ref8:
    case DW_FORM_ref4:
    case DW_FORM_ref2:
    case DW_FORM_ref1:
    case DW_FORM_GNU_ref_alt:
    case DW_FORM_ref_sig8:
      if (dwarf_formref_die (attr, ref) != NULL)
        {
          Dwarf_Off offset = dwarf_dieoffset (ref);
          const char *tag = DW_TAG__string (dwarf_tag (ref));
          *value = g_strdup_printf ("[%" G_GINT64_MODIFIER "x] %s",
                                    offset, tag);
          return *value;
        }
      return NULL;

    case DW_FORM_sdata:
      if (dwarf_formsdata (attr, &sword) == 0)
        {
          *value = g_strdup_printf ("%" G_GINT64_FORMAT, sword);
          return *value;
        }
      return NULL;

    case DW_FORM_sec_offset:
    case DW_FORM_udata:
    case DW_FORM_data8:
    case DW_FORM_data4:
    case DW_FORM_data2:
    case DW_FORM_data1:
      /* NOTE: readelf does extra parsing on these (and sdata too)
       * according to the known values for specific attributes.  */
      if (dwarf_formudata (attr, &word) == 0)
        {
          if (word < 0x10000) /* Print smallish constants in decimal.  */
            *value = g_strdup_printf ("%" G_GUINT64_FORMAT, word);
          else
            *value = g_strdup_printf ("%#" G_GINT64_MODIFIER "x", word);
          return *value;
        }
      return NULL;

    case DW_FORM_flag:
      if (dwarf_formflag (attr, &flag) == 0)
        return flag ? "yes" : "no";
      return NULL;

    case DW_FORM_flag_present:
      return "yes";

    case DW_FORM_block:
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_exprloc:
      /* TODO */

    default:
      return NULL;
    }
}


int
getattrs_callback (Dwarf_Attribute *attr, void *user_data)
{
  AttrCallback *data = (AttrCallback *)user_data;

  gtk_tree_store_insert_after (data->store, &data->iter,
                               data->parent, data->sibling);

  int code = dwarf_whatattr (attr);
  const char *codestring = DW_AT__string (code);

  int form = dwarf_whatform (attr);
  const char *formstring = DW_FORM__string (form);

  Dwarf_Die ref;
  ref.addr = NULL;
  char *value_mem = NULL;
  const char *value = attr_value (attr, &value_mem, &ref);

  gtk_tree_store_set (data->store, &data->iter,
                      ATTR_TREE_COL_ATTRIBUTE, codestring,
                      ATTR_TREE_COL_FORM, formstring,
                      ATTR_TREE_COL_VALUE, value,
                      -1);

  g_free (value_mem);

  if (ref.addr != NULL && code != DW_AT_sibling)
    {
      AttrCallback cbdata = *data;
      cbdata.parent = &data->iter;
      cbdata.sibling = NULL;
      dwarf_getattrs (&ref, getattrs_callback, &cbdata, 0);
    }

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
                                      ATTR_TREE_COL_FORM);

  col = gtk_tree_view_get_column (view, 2);
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
                                            G_TYPE_STRING, /* FORM */
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
