/*
 * die-tree view interface.
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

#include "session.h"


G_GNUC_INTERNAL
gboolean die_tree_view_render (GtkTreeView *view,
                               DwarvishSession *session,
                               gboolean types);

G_GNUC_INTERNAL
gboolean die_tree_get_die (GtkTreeModel *model,
                           GtkTreeIter *iter,
                           Dwarf_Die *die);


#endif /* _DIETREE_H_ */


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
