/*
 * Function declarations to load Dwfl instances.
 * Copyright (C) 2013  Josh Stone
 *
 * This file is part of dwarvish, and is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 */

#ifndef _LOADDWFL_H_
#define _LOADDWFL_H_

#include <glib.h>
#include <elfutils/libdwfl.h>

G_GNUC_INTERNAL
Dwfl *load_elf_dwfl (const char *file);

G_GNUC_INTERNAL
Dwfl *load_kernel_dwfl (const char *kernel, const char *module);

G_GNUC_INTERNAL
Dwfl_Module *get_first_module (Dwfl *dwfl);

G_GNUC_INTERNAL
const char *get_debugaltfile (Dwarf *dwarf);

#endif /* _LOADDWFL_H_ */


/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
