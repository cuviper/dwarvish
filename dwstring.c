/*
 * DWARF strings implementation.
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

#include <stdlib.h>
#include <dwarf.h>
#include "dwstring.h"

#define ONE_KNOWN_DW_DESC(name, code, desc) \
  ONE_KNOWN_DW (name, code)

#define ONE_KNOWN_DW(name, code) \
  case code: return #name;

#define ONE_KNOWN_DW_SET(set)   \
  const char *                  \
  DW_##set##__string (int code) \
  {                             \
    switch (code)               \
      {                         \
      ALL_KNOWN_DW_##set        \
      default: return NULL;     \
      }                         \
  }

ALL_KNOWN_DW_SETS

#undef ONE_KNOWN_DW_SET
#undef ONE_KNOWN_DW
#undef ONE_KNOWN_DW_DESC


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
