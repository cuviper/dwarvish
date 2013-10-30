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
#include "attrtree.h"
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
static G_DEFINE_SLICED_FREE (Dwarf_Die, dwarf_die);
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


static GString *
dwarf_die_typename_part (Dwarf_Die *die, Dwarf_Die *subroutine)
{
  const char *prepend = NULL, *append = NULL;
  switch (dwarf_tag (die))
    {
    case DW_TAG_base_type:
    case DW_TAG_typedef:
      return g_string_new (dwarf_diename (die));

    case DW_TAG_class_type:
      prepend = "class ";
      break;
    case DW_TAG_enumeration_type:
      prepend = "enum ";
      break;
    case DW_TAG_structure_type:
      prepend = "struct ";
      break;
    case DW_TAG_union_type:
      prepend = "union ";
      break;

    case DW_TAG_array_type:
      append = "[]";
      break;
    case DW_TAG_const_type:
      append = " const";
      break;
    case DW_TAG_pointer_type:
      append = "*";
      break;
    case DW_TAG_reference_type:
      append = "&";
      break;
    case DW_TAG_rvalue_reference_type:
      append = "&&";
      break;
    case DW_TAG_volatile_type:
      append = " volatile";
      break;

    case DW_TAG_subroutine_type:
      /* Subroutine types (function pointers) are a weird case.  The modifiers
       * we've recursed so far need to go in the middle, with the return type
       * on the left and parameter types on the right.  We'll back out now to
       * get those modifiers, getting the return and parameters separately.  */
      *subroutine = *die;
      return g_string_new ("");

    default:
      return NULL;
    }

  GString *string = NULL;
  if (prepend != NULL)
    {
      GString *string = g_string_new (prepend);
      return g_string_append (string, dwarf_diename (die) ?: "{...}");
    }

  Dwarf_Die type;
  Dwarf_Attribute attr;
  if (dwarf_attr (die, DW_AT_type, &attr) == NULL)
    string = g_string_new ("void");
  else if (dwarf_formref_die (&attr, &type) != NULL)
    string = dwarf_die_typename_part (&type, subroutine);

  if (string == NULL)
    return NULL;

  if (append != NULL)
    g_string_append (string, append);

  return string;
}


static GString *
dwarf_die_typename (Dwarf_Die *die)
{
  Dwarf_Die subroutine;
  subroutine.addr = 0;

  GString *string = dwarf_die_typename_part (die, &subroutine);
  if (string == NULL || subroutine.addr == 0)
    return string;

  /* Subroutine types need special handling.  First add the return value.  */
  GString *retval;
  Dwarf_Die type;
  Dwarf_Attribute attr;
  g_string_prepend (string, " (");
  if (dwarf_attr (&subroutine, DW_AT_type, &attr) == NULL)
    g_string_prepend (string, "void");
  else if (dwarf_formref_die (&attr, &type) != NULL &&
           (retval = dwarf_die_typename (&type)) != NULL)
    {
      g_string_prepend (string, retval->str);
      g_string_free (retval, TRUE);
    }
  else
    g_string_prepend (string, "?");
  g_string_append (string, ")");

  /* Now parameters.  */
  gboolean first = TRUE;
  g_string_append (string, " (");
  Dwarf_Die child;
  if (dwarf_child (&subroutine, &child) == 0)
    do
      if (dwarf_tag (&child) == DW_TAG_formal_parameter)
        {
          if (!first)
            g_string_append (string, ", ");
          else
            first = FALSE;

          GString *param;
          if (dwarf_attr (&child, DW_AT_type, &attr) == NULL)
            g_string_append (string, "void");
          else if (dwarf_formref_die (&attr, &type) != NULL &&
                   (param = dwarf_die_typename (&type)) != NULL)
            {
              g_string_append (string, param->str);
              g_string_free (param, TRUE);
            }
          else
            g_string_append (string, "?");
        }
      else if (dwarf_tag (&child) == DW_TAG_unspecified_parameters)
        {
          if (!first)
            g_string_append (string, ", ");
          else
            first = FALSE;
          g_string_append (string, "...");
        }
    while (dwarf_siblingof (&child, &child) == 0);
  if (first)
    g_string_append (string, "void");
  return g_string_append (string, ")");
}


static void
die_tree_set_die (GtkTreeStore *store, GtkTreeIter *iter,
                  Dwarf_Die *die, const char *name)
{
  gchar *offset = g_strdup_printf ("%" G_GINT64_MODIFIER "x",
                                   dwarf_dieoffset (die));
  gchar *tag = DW_TAG__strdup_hex (dwarf_tag (die));

  GString *typename = NULL;
  if (name == NULL)
    {
      typename = dwarf_die_typename (die);
      name = (typename != NULL) ? typename->str : dwarf_diename (die);
    }

  gtk_tree_store_set (store, iter,
                      DIE_TREE_COL_OFFSET, offset,
                      DIE_TREE_COL_TAG, tag,
                      DIE_TREE_COL_NAME, name,
                      DIE_TREE_INT_DIE, die,
                      -1);

  g_free (offset);
  g_free (tag);
  if (typename != NULL)
    g_string_free (typename, TRUE);

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
        die_tree_set_die (store, iter, &child, NULL);
        sibling = iter;
      }
    while (dwarf_siblingof (&child, &child) == 0);
  return sibling;
}


static gboolean
die_tree_iter_is_leaf (GtkTreeModel *model, GtkTreeIter *iter)
{
  GtkTreeIter first_child;
  if (!gtk_tree_model_iter_children (model, &first_child, iter))
    return TRUE;

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
signal_die_tree_test_expand_row (GtkTreeView *tree_view, GtkTreeIter *iter,
                                 G_GNUC_UNUSED GtkTreePath *path,
                                 G_GNUC_UNUSED gpointer user_data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  return die_tree_iter_is_leaf (model, iter);
}


/* Search depth-first for a matching die, and return its path.  */
static GtkTreePath *
die_tree_search_down (GtkTreeModel *model, Dwarf_Die *search_die,
                      GtkTreeIter *iter)
{
  Dwarf_Die die;
  if (die_tree_get_die (model, iter, &die) &&
      die.addr == search_die->addr)
    return gtk_tree_model_get_path (model, iter);

  GtkTreeIter child;
  if (die_tree_iter_is_leaf (model, iter) ||
      !gtk_tree_model_iter_children (model, &child, iter))
    return NULL;

  GtkTreePath *path;
  do
    path = die_tree_search_down (model, search_die, &child);
  while (path == NULL && gtk_tree_model_iter_next (model, &child));
  return path;
}


/* Search depth-first for a matching die, retrying upwards if needed.  */
static GtkTreePath *
die_tree_search_up (GtkTreeModel *model, Dwarf_Die *search_die,
                    GtkTreeIter *iter)
{
  /* First look downward.  */
  GtkTreePath *path = die_tree_search_down (model, search_die, iter);
  if (path != NULL)
    return path;

  /* No luck, then try again from this parent.
   * NB: As written, this will not cross into other CUs.  */
  GtkTreeIter parent;
  if (gtk_tree_model_iter_parent (model, &parent, iter))
    return die_tree_search_up (model, search_die, &parent);

  return NULL;
}


/* When a "ref" attribute is activated (enter / double-click), find the
 * corresponding DIE in its tree and relocate the cursor there.  */
G_MODULE_EXPORT void
signal_attr_tree_row_activated (GtkTreeView *attrview,
                                GtkTreePath *path,
                                G_GNUC_UNUSED GtkTreeViewColumn *column,
                                gpointer user_data)
{
  GtkTreeView *view = GTK_TREE_VIEW (user_data);
  GtkTreeModel *model = gtk_tree_view_get_model (view);

  /* First read the DIE from the attribute.  */
  Dwarf_Attribute attr;
  Dwarf_Die die;
  GtkTreeIter iter;
  GtkTreeModel *attrmodel = gtk_tree_view_get_model (attrview);
  if (!gtk_tree_model_get_iter (attrmodel, &iter, path)
      || !attr_tree_get_attribute (attrmodel, &iter, &attr)
      || !dwarf_formref_die (&attr, &die))
    return;

  /* Now search for the same die->addr in the die tree.  It's a depth-first
   * search which starts on the current die cursor.  If that fails it tries
   * again from the parent, etc.  That way we can hopefully keep the lazy
   * expansion to a minimum.  */
  GtkTreePath *cursor_path;
  gtk_tree_view_get_cursor (view, &cursor_path, NULL);
  if (gtk_tree_model_get_iter (model, &iter, cursor_path))
    {
      GtkTreePath *diepath = die_tree_search_up (model, &die, &iter);
      if (diepath != NULL)
        {
          /* Expand nodes up to but not including the target.  */
          GtkTreePath *parentpath = gtk_tree_path_copy (diepath);
          gtk_tree_path_up (parentpath);
          gtk_tree_view_expand_to_path (view, parentpath);
          gtk_tree_path_free (parentpath);

          /* Select the target.  */
          gtk_tree_view_set_cursor (view, diepath, NULL, FALSE);
          gtk_tree_path_free (diepath);
        }
    }
  gtk_tree_path_free (cursor_path);
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

      char *sig8_name = NULL;
      const char *name = dwarf_diename (&die);
      if (name == NULL && types)
        name = sig8_name = g_strdup_printf ("{%016" G_GINT64_MODIFIER "x}",
                                            type_signature);

      empty = FALSE;
      gtk_tree_store_insert_after (store, &iter, NULL, sibling);
      die_tree_set_die (store, &iter, &die, name);
      sibling = &iter;

      if (sig8_name)
        g_free (sig8_name);
    }

  gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
  g_object_unref (store); /* The view keeps its own reference.  */

  die_tree_render_column (view, DIE_TREE_COL_OFFSET);
  die_tree_render_column (view, DIE_TREE_COL_TAG);
  die_tree_render_column (view, DIE_TREE_COL_NAME);

  return !empty;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
