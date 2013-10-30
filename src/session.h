/*
 * Common session object
 * Copyright (C) 2013  Josh Stone
 *
 * This file is part of dwarvish, and is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 */

#ifndef __SESSION_H__
#define __SESSION_H__

#include <elfutils/libdwfl.h>
#include <glib.h>


typedef struct _DwarvishSession
{
  /* User options.  */
  gboolean nested_imports;
  gboolean explicit_imports;
  gboolean explicit_siblings;
  gchar *kernel;
  gchar *module;
  gchar *file;

  /* Elfutils objects.  */
  Dwfl *dwfl;
  Dwfl_Module *dwflmod;
  Dwarf *dwarf;

  /* Additional metadata.  */
  gchar *basename;
  gchar *mainfile;
  gchar *debugfile;
  gchar *debugaltfile;
} DwarvishSession;


#endif /* __SESSION_H__ */


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
