/*
 * DWARF strings interface.
 * Copyright (C) 2013  Josh Stone
 *
 * This file is part of dwarvish, and is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 */

#ifndef __DW_STRING_H__
#define __DW_STRING_H__

#include <glib.h>
#include "known-dwarf.h"

#define ONE_KNOWN_DW_SET(set) \
  G_GNUC_INTERNAL const char *DW_##set##__string (int code);
ALL_KNOWN_DW_SETS
#undef ONE_KNOWN_DW_SET

#endif /* __DW_STRING_H__ */


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
