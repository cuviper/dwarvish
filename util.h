/*
 * Miscellaneous functions and macros.
 * Copyright (C) 2013  Josh Stone
 *
 * This file is part of dwarvish, and is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <glib.h>


#define G_DEFINE_SLICED_COPY_FREE(TypeName, type_name)  \
static gpointer                                         \
type_name##_copy (gpointer boxed)                       \
{                                                       \
  return g_slice_dup (TypeName, boxed);                 \
}                                                       \
static void                                             \
type_name##_free (gpointer boxed)                       \
{                                                       \
  g_slice_free (TypeName, boxed);                       \
}

#define G_DEFINE_SLICED_BOXED_TYPE(TypeName, type_name) \
G_DEFINE_BOXED_TYPE (TypeName, type_name, type_name##_copy, type_name##_free)


#endif /* __UTIL_H__ */


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
