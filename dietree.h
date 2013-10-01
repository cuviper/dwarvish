/*
 * die-tree model interface.
 * Copyright (C) 2013  Josh Stone
 *
 * This file is part of dwarvish, and is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 */

#ifndef _DIETREE_H_
#define _DIETREE_H_

#include <elfutils/libdw.h>
#include <gtk/gtk.h>


/* The data columns that we export via the tree model interface */
enum
{
  DIE_TREE_COL_OFFSET = 0,
  DIE_TREE_COL_NAME,
  DIE_TREE_COL_TAG,
  DIE_TREE_COL_TAG_STRING,
  DIE_TREE_N_COLUMNS,
};

GtkTreeModel *die_tree_new (Dwarf *dwarf, gboolean types);

#endif /* _DIETREE_H_ */


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
