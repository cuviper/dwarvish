/*
 * die-tree model implementation.
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


/* Standard GObject boilerplate.  */
#define TYPE_DIE_TREE           (die_tree_get_type ())
#define DIE_TREE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DIE_TREE, DieTree))
#define IS_DIE_TREE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_DIE_TREE))
#define DIE_TREE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST ((cls), TYPE_DIE_TREE, DieTreeClass))
#define IS_DIE_TREE_CLASS(cls)  (G_TYPE_CHECK_CLASS_TYPE ((cls), TYPE_DIE_TREE))
#define DIE_TREE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_DIE_TREE, DieTreeClass))

typedef struct _DieTree
{
  GObject parent;
  Dwarf *dwarf;
  gboolean types;
  GNode *root;
  gint stamp;
} DieTree;

typedef struct _DieTreeClass
{
  GObjectClass parent_class;
} DieTreeClass;

static void die_tree_model_init (GtkTreeModelIface *iface);

G_DEFINE_TYPE_WITH_CODE (DieTree, die_tree, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, die_tree_model_init));


static const GType DieTreeColumnTypes[DIE_TREE_N_COLUMNS] =
{
  [DIE_TREE_COL_OFFSET] = G_TYPE_UINT64,
  [DIE_TREE_COL_NAME] = G_TYPE_STRING,
  [DIE_TREE_COL_TAG] = G_TYPE_INT,
  [DIE_TREE_COL_TAG_STRING] = G_TYPE_STRING,
};


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


/* Technically, Dwarf_Off is 64-bit, so converting to a 32-bit gpointer would
 * be lossy.  However, I happen to know that elfutils mmaps the debug files,
 * so it's limited to virtual memory size already.  */
static inline gpointer
off2ptr (Dwarf_Off off)
{
  return (gpointer)(gintptr)off;
}

static inline Dwarf_Off
ptr2off (gpointer ptr)
{
  return (Dwarf_Off)(gintptr)ptr;
}

static inline gpointer
die2ptr (Dwarf_Die *die)
{
  return off2ptr (dwarf_dieoffset (die));
}

static inline gboolean
ptr2die (DieTree *dietree, gpointer ptr, Dwarf_Die *die)
{
  Dwarf_Off off = ptr2off (ptr);
  Dwarf_Die *res = dietree->types ?
    dwarf_offdie_types (dietree->dwarf, off, die)
    : dwarf_offdie (dietree->dwarf, off, die);
  return (res != NULL);
}


static GtkTreeModelFlags
die_tree_get_flags (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), 0);

  return (GTK_TREE_MODEL_ITERS_PERSIST);
}


static gint
die_tree_get_n_columns (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), 0);

  return DIE_TREE_N_COLUMNS;
}


static GType
die_tree_get_column_type (GtkTreeModel *tree_model, gint index)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), G_TYPE_INVALID);
  g_return_val_if_fail (index >= 0, G_TYPE_INVALID);
  g_return_val_if_fail (index < DIE_TREE_N_COLUMNS, G_TYPE_INVALID);

  return DieTreeColumnTypes[index];
}


/* Lazy function to expand DIE children.  */
static void
die_tree_get_children (DieTree *dietree, GNode *node)
{
  /* The root is handled differently (dwarf_nextcu/dwarf_next_unit),
   * but that was done immediately in die_tree_new.  */
  if (G_NODE_IS_ROOT (node))
    return;

  /* Check if this already has its children.  */
  if (!G_NODE_IS_LEAF (node))
    return;

  Dwarf_Die die;
  if (!ptr2die (dietree, node->data, &die))
    g_return_if_reached ();

  if (dwarf_child (&die, &die) == 0)
    do
      g_node_append_data (node, die2ptr (&die));
    while (dwarf_siblingof (&die, &die) == 0);
}


static gboolean
die_tree_get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter,
                   GtkTreePath *path)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  DieTree *dietree = DIE_TREE (tree_model);

  gint *indices = gtk_tree_path_get_indices (path);
  gint depth = gtk_tree_path_get_depth (path);

  GNode *node = dietree->root;
  for (gint i = 0; node && i < depth; ++i)
    {
      die_tree_get_children (dietree, node);
      node = g_node_nth_child (node, indices[i]);
    }

  if (depth == 0 || !node)
    return FALSE;

  iter->stamp = dietree->stamp;
  iter->user_data = node;
  return TRUE;
}


static GtkTreePath *
die_tree_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), NULL);
  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (iter->user_data != NULL, NULL);

  DieTree *dietree = DIE_TREE (tree_model);
  g_return_val_if_fail (iter->stamp == dietree->stamp, NULL);

  GtkTreePath *path = gtk_tree_path_new ();
  for (GNode *node = iter->user_data;
       !G_NODE_IS_ROOT (node); node = node->parent)
    {
      gint pos = g_node_child_position (node->parent, node);
      gtk_tree_path_prepend_index (path, pos);
    }

  return path;
}


static void
die_tree_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter,
                    gint column, GValue *value)
{
  g_return_if_fail (IS_DIE_TREE (tree_model));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (column >= 0);
  g_return_if_fail (column < DIE_TREE_N_COLUMNS);

  DieTree *dietree = DIE_TREE (tree_model);
  g_return_if_fail (iter->stamp == dietree->stamp);

  g_value_init (value, DieTreeColumnTypes[column]);

  GNode *node = iter->user_data;
  Dwarf_Off offset = ptr2off (node->data);
  if (column == DIE_TREE_COL_OFFSET)
    {
      g_value_set_uint64 (value, offset);
      return;
    }

  Dwarf_Die die;
  if (!ptr2die (dietree, node->data, &die))
    g_return_if_reached ();

  switch (column)
    {
    case DIE_TREE_COL_NAME:
      g_value_set_static_string (value, dwarf_diename (&die));
      break;

    case DIE_TREE_COL_TAG:
      g_value_set_int (value, dwarf_tag (&die));
      break;

    case DIE_TREE_COL_TAG_STRING:
        {
          int tag = dwarf_tag (&die);
          const char *tagname = dwarf_tag_string (tag);
          if (tagname)
            g_value_set_static_string (value, tagname);
          else
            {
              gchar *text = g_strdup_printf ("unknown(%d)", tag);
              g_value_take_string (value, text);
            }
        }
      break;
    }
}


static gboolean
die_tree_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  DieTree *dietree = DIE_TREE (tree_model);
  g_return_val_if_fail (iter->stamp == dietree->stamp, FALSE);

  GNode *next = g_node_next_sibling (iter->user_data);
  if (!next)
    return FALSE;

  iter->stamp = dietree->stamp;
  iter->user_data = next;
  return TRUE;
}


static gboolean
die_tree_iter_previous (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  DieTree *dietree = DIE_TREE (tree_model);
  g_return_val_if_fail (iter->stamp == dietree->stamp, FALSE);

  GNode *previous = g_node_prev_sibling (iter->user_data);
  if (!previous)
    return FALSE;

  iter->stamp = dietree->stamp;
  iter->user_data = previous;
  return TRUE;
}


static gboolean
die_tree_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter,
                        GtkTreeIter *parent)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), FALSE);

  DieTree *dietree = DIE_TREE (tree_model);

  GNode *node;
  if (parent)
    {
      g_return_val_if_fail (parent->stamp == dietree->stamp, FALSE);
      node = parent->user_data;
    }
  else
    node = dietree->root;

  die_tree_get_children (dietree, node);
  GNode *child = g_node_first_child (node);

  if (!child)
    return FALSE;

  iter->stamp = dietree->stamp;
  iter->user_data = child;
  return TRUE;
}


static gboolean
die_tree_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  DieTree *dietree = DIE_TREE (tree_model);
  g_return_val_if_fail (iter->stamp == dietree->stamp, FALSE);

  GNode *node = iter->user_data;
  if (!G_NODE_IS_LEAF (node))
    return TRUE;

  Dwarf_Die die;
  if (!ptr2die (dietree, node->data, &die))
    g_return_val_if_reached (FALSE);

  return dwarf_haschildren (&die);
}


static gint
die_tree_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), -1);

  DieTree *dietree = DIE_TREE (tree_model);

  GNode *node;
  if (iter)
    {
      g_return_val_if_fail (iter->stamp == dietree->stamp, FALSE);
      node = iter->user_data;
    }
  else
    node = dietree->root;

  die_tree_get_children (dietree, node);
  return g_node_n_children (node);
}


static gboolean
die_tree_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter,
                         GtkTreeIter *parent, gint n)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), FALSE);

  DieTree *dietree = DIE_TREE (tree_model);

  GNode *node;
  if (parent)
    {
      g_return_val_if_fail (parent->stamp == dietree->stamp, FALSE);
      node = parent->user_data;
    }
  else
    node = dietree->root;

  die_tree_get_children (dietree, node);
  GNode *child = g_node_nth_child (node, n);
  if (!child)
    return FALSE;

  iter->stamp = dietree->stamp;
  iter->user_data = child;
  return TRUE;
}


static gboolean
die_tree_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter,
                      GtkTreeIter *child)
{
  g_return_val_if_fail (IS_DIE_TREE (tree_model), FALSE);

  DieTree *dietree = DIE_TREE (tree_model);
  g_return_val_if_fail (child->stamp == dietree->stamp, FALSE);

  GNode *node = child->user_data;

  GNode *parent = node->parent;
  if (G_NODE_IS_ROOT (parent))
    return FALSE;

  iter->stamp = dietree->stamp;
  iter->user_data = parent;
  return TRUE;
}


GtkTreeModel *
die_tree_new (Dwarf *dwarf, gboolean types)
{
  g_return_val_if_fail (dwarf != NULL, NULL);

  DieTree *dietree = g_object_new (TYPE_DIE_TREE, NULL);
  g_assert (dietree != NULL);

  dietree->dwarf = dwarf;
  dietree->types = types;

  size_t cuhl;
  uint64_t type_signature;
  Dwarf_Off noff, off = 0;
  while (dwarf_next_unit (dwarf, off, &noff, &cuhl, NULL, NULL, NULL, NULL,
                          (types ? &type_signature : NULL), NULL) == 0)
    {
      g_node_append_data (dietree->root, off2ptr (off + cuhl));
      off = noff;
    }

  return GTK_TREE_MODEL (dietree);
}


static void
die_tree_finalize (GObject *obj)
{
  DieTree *dietree = DIE_TREE (obj);
  g_node_destroy (dietree->root);
  G_OBJECT_CLASS (die_tree_parent_class)->finalize (obj);
}


static void
die_tree_init (DieTree *dietree)
{
  do
    dietree->stamp = g_random_int ();
  while (dietree->stamp == 0);
  dietree->root = g_node_new (NULL);
  dietree->dwarf = NULL;
}


static void
die_tree_class_init (DieTreeClass *cls)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (cls);
  gobject_class->finalize = die_tree_finalize;
}


static void
die_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags       = die_tree_get_flags;
  iface->get_n_columns   = die_tree_get_n_columns;
  iface->get_column_type = die_tree_get_column_type;
  iface->get_iter        = die_tree_get_iter;
  iface->get_path        = die_tree_get_path;
  iface->get_value       = die_tree_get_value;
  iface->iter_next       = die_tree_iter_next;
  iface->iter_previous   = die_tree_iter_previous;
  iface->iter_children   = die_tree_iter_children;
  iface->iter_has_child  = die_tree_iter_has_child;
  iface->iter_n_children = die_tree_iter_n_children;
  iface->iter_nth_child  = die_tree_iter_nth_child;
  iface->iter_parent     = die_tree_iter_parent;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
