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


static gpointer
dwarf_attribute_copy (gpointer boxed)
{
  return g_memdup (boxed, sizeof (Dwarf_Attribute));
}
static G_DEFINE_BOXED_TYPE (Dwarf_Attribute, dwarf_attribute,
                            dwarf_attribute_copy, g_free);
#define G_TYPE_DWARF_ATTRIBUTE (dwarf_attribute_get_type ())


gboolean
attr_tree_get_attribute (GtkTreeModel *model, GtkTreeIter *iter,
                         Dwarf_Attribute *attr_mem)
{
  Dwarf_Attribute *attr = NULL;
  gtk_tree_model_get (model, iter, 0, &attr, -1);
  if (attr == NULL)
    return FALSE;
  *attr_mem = *attr;
  g_free (attr);
  return TRUE;
}


const char *
attr_value_string (Dwarf_Attribute *attr, char **alloc)
{
  bool flag;
  Dwarf_Die ref;
  Dwarf_Addr addr;
  Dwarf_Sword sword;
  Dwarf_Word word;

  switch (dwarf_whatform (attr))
    {
    case DW_FORM_addr:
      if (dwarf_formaddr (attr, &addr) == 0)
        {
          *alloc = g_strdup_printf ("%#" G_GINT64_MODIFIER "x", addr);
          return *alloc;
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
      if (dwarf_formref_die (attr, &ref) != NULL)
        {
          Dwarf_Off offset = dwarf_dieoffset (&ref);
          const char *tag = DW_TAG__string (dwarf_tag (&ref));
          *alloc = g_strdup_printf ("[%" G_GINT64_MODIFIER "x] %s",
                                    offset, tag);
          return *alloc;
        }
      return NULL;

    case DW_FORM_sdata:
      if (dwarf_formsdata (attr, &sword) == 0)
        {
          *alloc = g_strdup_printf ("%" G_GINT64_FORMAT, sword);
          return *alloc;
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
            *alloc = g_strdup_printf ("%" G_GUINT64_FORMAT, word);
          else
            *alloc = g_strdup_printf ("%#" G_GINT64_MODIFIER "x", word);
          return *alloc;
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
  gtk_tree_store_set (data->store, &data->iter, 0, attr, -1);

  Dwarf_Die ref;
  if (dwarf_whatattr (attr) != DW_AT_sibling
      && dwarf_formref_die (attr, &ref) != NULL)
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
  GtkTreeView *view = GTK_TREE_VIEW (user_data);
  GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));
  gtk_tree_store_clear (store);

  Dwarf_Die die;
  GtkTreeIter iter;
  GtkTreeModel *model;
  if (!gtk_tree_selection_get_selected (selection, &model, &iter)
      || !die_tree_get_die (model, &iter, &die))
    return;

  AttrCallback cbdata;
  cbdata.store = store;
  cbdata.parent = NULL;
  cbdata.sibling = NULL;
  dwarf_getattrs (&die, getattrs_callback, &cbdata, 0);
}


enum
{
  ATTR_TREE_COL_ATTRIBUTE,
  ATTR_TREE_COL_FORM,
  ATTR_TREE_COL_VALUE,
};


static void
attr_tree_cell_data (GtkTreeViewColumn *column __attribute__ ((unused)),
                     GtkCellRenderer *cell, GtkTreeModel *model,
                     GtkTreeIter *iter, gpointer data)
{
  Dwarf_Attribute attr;
  if (!attr_tree_get_attribute (model, iter, &attr))
    g_return_if_reached ();

  gchar *alloc = NULL;
  const gchar *fixed = NULL;
  switch ((gintptr)data)
    {
    case ATTR_TREE_COL_ATTRIBUTE:
      fixed = DW_AT__string (dwarf_whatattr (&attr));
      break;

    case ATTR_TREE_COL_FORM:
      fixed = DW_FORM__string (dwarf_whatform (&attr));
      break;

    case ATTR_TREE_COL_VALUE:
      fixed = attr_value_string (&attr, &alloc);
      break;
    }

  g_object_set (cell, "text", alloc ?: fixed, NULL);
  g_free (alloc);
}


static void
attr_tree_render_column (GtkTreeView *view, gint column, gint virtual_column)
{
  GtkTreeViewColumn *col = gtk_tree_view_get_column (view, column);
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "font", "monospace 9", NULL);

  gpointer data = (gpointer)(gintptr)virtual_column;
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (col, renderer,
                                           attr_tree_cell_data,
                                           data, NULL);
}


gboolean
attr_tree_view_render (GtkTreeView *dietree, GtkTreeView *attrtree)
{
  GtkTreeStore *store = gtk_tree_store_new (1, G_TYPE_DWARF_ATTRIBUTE);

  gtk_tree_view_set_model (attrtree, GTK_TREE_MODEL (store));
  g_object_unref (store); /* The view keeps its own reference.  */

  attr_tree_render_column (attrtree, 0, ATTR_TREE_COL_ATTRIBUTE);
  attr_tree_render_column (attrtree, 1, ATTR_TREE_COL_FORM);
  attr_tree_render_column (attrtree, 2, ATTR_TREE_COL_VALUE);

  g_signal_connect (gtk_tree_view_get_selection (dietree), "changed",
                    G_CALLBACK (die_tree_selection_changed), attrtree);

  return TRUE;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
