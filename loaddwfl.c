/*
 * Functions to load Dwfl instances.
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

#include <glib.h>

#include "loaddwfl.h"


Dwfl *
load_elf_dwfl (const char *file)
{
  (void)file;
  static const Dwfl_Callbacks elf_callbacks =
    {
      NULL,
      dwfl_standard_find_debuginfo,
      dwfl_offline_section_address,
      NULL
    };

  Dwfl *dwfl = dwfl_begin (&elf_callbacks);
  dwfl_report_begin (dwfl);

  Dwfl_Module *mod = dwfl_report_offline (dwfl, file, file, -1);

  dwfl_report_end (dwfl, NULL, NULL);
  if (mod != NULL)
    return dwfl;

  dwfl_end (dwfl);
  return NULL;
}


static const char *search_module;

static int
module_predicate (const char *module, G_GNUC_UNUSED const char *path)
{
  if (search_module == NULL)
    return -1;

  if (g_strcmp0 (search_module, module) == 0)
    {
      search_module = NULL;
      return 1;
    }

  return 0;
}


Dwfl *
load_kernel_dwfl (const char *kernel, const char *module)
{
  (void)kernel;
  (void)module;
  static const Dwfl_Callbacks kernel_callbacks =
    {
      dwfl_linux_kernel_find_elf,
      dwfl_standard_find_debuginfo,
      dwfl_offline_section_address,
      NULL
    };

  Dwfl *dwfl = dwfl_begin (&kernel_callbacks);
  dwfl_report_begin (dwfl);

  search_module = module ?: "kernel";
  dwfl_linux_kernel_report_offline (dwfl, kernel, module_predicate);

  dwfl_report_end (dwfl, NULL, NULL);
  if (search_module == NULL)
    return dwfl;

  dwfl_end (dwfl);
  return NULL;
}


static int
get_first_module_cb (Dwfl_Module *mod,
                     G_GNUC_UNUSED void **userdata,
                     G_GNUC_UNUSED const char *name,
                     G_GNUC_UNUSED Dwarf_Addr start,
                     void *arg)
{
  *(Dwfl_Module**)arg = mod;
  return DWARF_CB_ABORT;
}


Dwfl_Module *
get_first_module (Dwfl *dwfl)
{
  Dwfl_Module *mod = NULL;
  dwfl_getmodules (dwfl, get_first_module_cb, &mod, 0);
  return mod;
}


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
