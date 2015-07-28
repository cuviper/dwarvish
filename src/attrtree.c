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
#include "util.h"


enum
{
  ATTR_TREE_COL_ATTRIBUTE = 0,
  ATTR_TREE_COL_FORM,
  ATTR_TREE_COL_VALUE,
  ATTR_TREE_INT_DIE,
  ATTR_TREE_INT_ATTR,
  ATTR_TREE_N_COLUMNS
};


static G_DEFINE_SLICED_COPY (Dwarf_Attribute, dwarf_attr);
static G_DEFINE_SLICED_FREE (Dwarf_Attribute, dwarf_attr);
static G_DEFINE_SLICED_BOXED_TYPE (Dwarf_Attribute, dwarf_attr);
#define G_TYPE_DWARF_ATTR (dwarf_attr_get_type ())


gboolean
attr_tree_get_attribute (GtkTreeModel *model, GtkTreeIter *iter,
                         Dwarf_Attribute *attr_mem)
{
  Dwarf_Attribute *attr = NULL;
  gtk_tree_model_get (model, iter, ATTR_TREE_INT_ATTR, &attr, -1);
  if (attr == NULL)
    return FALSE;
  *attr_mem = *attr;
  dwarf_attr_free (attr);
  return TRUE;
}


/* Like dwarf_decl_file for an already known DW_AT_decl_file.  This might be
 * a bit paranoid, but it's more directly showing what's specified.  */
static const char *
dwarf_die_file_idx (Dwarf_Die *die, size_t idx)
{
  Dwarf_Die cu;
  Dwarf_Files *files;
  if (dwarf_diecu (die, &cu, NULL, NULL) != NULL &&
      dwarf_getsrcfiles (&cu, &files, NULL) == 0)
    return dwarf_filesrc (files, idx, NULL, NULL);
  return NULL;
}


/* Print special cases of data attributes.  */
static char *
attr_value_data_string (Dwarf_Die *die, Dwarf_Attribute *attr)
{
  Dwarf_Word udata;
  if (dwarf_formudata (attr, &udata) != 0)
    return NULL;

  const char *str = NULL;

  switch (dwarf_whatattr (attr))
    {
    case DW_AT_call_file:
    case DW_AT_decl_file:
      str = dwarf_die_file_idx (die, udata);
      if (str != NULL)
        return g_strdup (str);
      break;

    case DW_AT_accessibility:
      return DW_ACCESS__strdup_hex (udata);

    case DW_AT_encoding:
      return DW_ATE__strdup_hex (udata);

    case DW_AT_calling_convention:
      return DW_CC__strdup_hex (udata);

    case DW_AT_decimal_sign:
      return DW_DS__strdup_hex (udata);

    case DW_AT_discr_value:
      return DW_DSC__strdup_hex (udata);

    case DW_AT_identifier_case:
      return DW_ID__strdup_hex (udata);

    case DW_AT_endianity:
      return DW_END__strdup_hex (udata);

    case DW_AT_inline:
      return DW_INL__strdup_hex (udata);

    case DW_AT_language:
      return DW_LANG__strdup_hex (udata);

    case DW_AT_ordering:
      return DW_ORD__strdup_hex (udata);

    case DW_AT_virtuality:
      return DW_VIRTUALITY__strdup_hex (udata);

    case DW_AT_visibility:
      return DW_VIS__strdup_hex (udata);

    default:
      break;
    }

  /* Print signed and small-unsigned constants in decimal.  */
  if (udata < 0x10000 || dwarf_whatform (attr) == DW_FORM_sdata)
    return g_strdup_printf ("%" G_GINT64_FORMAT, udata);

  return g_strdup_printf ("%#" G_GINT64_MODIFIER "x", udata);
}


static char *
attr_value_sec_offset_string (Dwarf_Attribute *attr)
{
  Dwarf_Word udata;
  if (dwarf_formudata (attr, &udata) != 0)
    return NULL;

  /* For now, just print section offsets as a [ref].  */
  return g_strdup_printf ("[%" G_GINT64_MODIFIER "x]", udata);
}


static char *
attr_value_string (Dwarf_Die *die, Dwarf_Attribute *attr)
{
  bool flag;
  Dwarf_Die ref;
  Dwarf_Addr addr;

  const char *str = NULL;

  switch (dwarf_whatform (attr))
    {
    case DW_FORM_addr:
      if (dwarf_formaddr (attr, &addr) == 0)
        return g_strdup_printf ("%#" G_GINT64_MODIFIER "x", addr);
      return NULL;

    case DW_FORM_indirect:
    case DW_FORM_strp:
    case DW_FORM_string:
    case DW_FORM_GNU_strp_alt:
      str = dwarf_formstring (attr);
      return (str != NULL) ? g_strdup (str) : NULL;

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
          if (G_LIKELY (tag != NULL))
            return g_strdup_printf ("[%" G_GINT64_MODIFIER "x] %s",
                                    offset, tag);
          return g_strdup_printf ("[%" G_GINT64_MODIFIER "x] %#x",
                                  offset, dwarf_tag (&ref));
        }
      return NULL;

    case DW_FORM_sec_offset:
      return attr_value_sec_offset_string (attr);

    case DW_FORM_udata:
    case DW_FORM_data8:
    case DW_FORM_data4:
    case DW_FORM_data2:
    case DW_FORM_data1:
    case DW_FORM_sdata:
      return attr_value_data_string (die, attr);

    case DW_FORM_flag:
      if (dwarf_formflag (attr, &flag) == 0)
        return g_strdup (flag ? "yes" : "no");
      return NULL;

    case DW_FORM_flag_present:
      return g_strdup ("yes");

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
  gboolean explicit_siblings;
  Dwarf_Die *die;
  GtkTreeStore *store;
  GtkTreeIter *parent;
  GtkTreeIter *sibling;
  GtkTreeIter iter;
  struct _AttrCallback *back;
} AttrCallback;


static int
getattrs_callback (Dwarf_Attribute *attr, void *user_data)
{
  AttrCallback *data = (AttrCallback *)user_data;
  Dwarf_Die *die = data->die;

  /* Siblings are hidden by default, as they're not really information
   * about the selected DIE itself.  */
  gboolean is_sibling = (dwarf_whatattr (attr) == DW_AT_sibling);
  if (is_sibling && !data->explicit_siblings)
    return DWARF_CB_OK;

  gchar *attribute = DW_AT__strdup_hex (dwarf_whatattr (attr));
  gchar *form = DW_FORM__strdup_hex (dwarf_whatform (attr));
  gchar *value = attr_value_string (die, attr);

  gtk_tree_store_insert_after (data->store, &data->iter,
                               data->parent, data->sibling);
  gtk_tree_store_set (data->store, &data->iter,
                      ATTR_TREE_COL_ATTRIBUTE, attribute,
                      ATTR_TREE_COL_FORM, form,
                      ATTR_TREE_COL_VALUE, value,
                      ATTR_TREE_INT_DIE, die,
                      ATTR_TREE_INT_ATTR, attr,
                      -1);

  g_free (attribute);
  g_free (form);
  g_free (value);

  /* Even when sibling attributes are explicitly shown, don't recurse on them,
   * as they're already shown as neighbors in the die tree.  */
  Dwarf_Die ref;
  if (!is_sibling && dwarf_formref_die (attr, &ref) != NULL)
    {
      /* Scan backwards to avoid expanding cycles.  */
      AttrCallback *match = data;
      while (match && match->die->addr != ref.addr)
        match = match->back;

      if (match == NULL)
        {
          AttrCallback cbdata = *data;
          cbdata.die = &ref;
          cbdata.parent = &data->iter;
          cbdata.sibling = NULL;
          cbdata.back = data;
          dwarf_getattrs (&ref, getattrs_callback, &cbdata, 0);
        }
    }

  data->sibling = &data->iter;
  return DWARF_CB_OK;
}


G_MODULE_EXPORT void
signal_die_tree_selection_changed (GtkTreeSelection *selection,
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

  DwarvishSession *session = g_object_get_data (G_OBJECT (model),
                                                "DwarvishSession");

  AttrCallback cbdata;
  cbdata.explicit_siblings = session->explicit_siblings;
  cbdata.die = &die;
  cbdata.store = store;
  cbdata.parent = NULL;
  cbdata.sibling = NULL;
  cbdata.back = NULL;
  dwarf_getattrs (&die, getattrs_callback, &cbdata, 0);
  gtk_tree_view_expand_all (view);
}


G_MODULE_EXPORT gboolean
signal_attr_tree_query_tooltip (GtkWidget *widget,
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

  gchar *value = NULL;
  gtk_tree_model_get (model, &iter, ATTR_TREE_COL_VALUE, &value, -1);
  if (value != NULL)
    {
      gtk_tooltip_set_text (tooltip, value);
      gtk_tree_view_set_tooltip_row (view, tooltip, path);
      g_free (value);
    }

  gtk_tree_path_free (path);

  return (value != NULL);
}


static void
attr_tree_render_column (GtkTreeView *view, gint column)
{
  GtkTreeViewColumn *col = gtk_tree_view_get_column (view, column);
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "font", "monospace 9", NULL);
  if (column == ATTR_TREE_COL_VALUE)
    g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text", column);
}


gboolean
attr_tree_view_render (GtkTreeView *attrtree, DwarvishSession *session)
{
  GtkTreeStore *store = gtk_tree_store_new (ATTR_TREE_N_COLUMNS,
                                            G_TYPE_STRING,
                                            G_TYPE_STRING,
                                            G_TYPE_STRING,
                                            G_TYPE_DWARF_DIE,
                                            G_TYPE_DWARF_ATTR);
  g_object_set_data (G_OBJECT (store), "DwarvishSession", session);

  gtk_tree_view_set_model (attrtree, GTK_TREE_MODEL (store));
  g_object_unref (store); /* The view keeps its own reference.  */

  attr_tree_render_column (attrtree, ATTR_TREE_COL_ATTRIBUTE);
  attr_tree_render_column (attrtree, ATTR_TREE_COL_FORM);
  attr_tree_render_column (attrtree, ATTR_TREE_COL_VALUE);

  return TRUE;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
