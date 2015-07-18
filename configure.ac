AC_PREREQ([2.69])
AC_INIT([Dwarvish], [0.1], [https://github.com/cuviper/dwarvish/issues],
        [dwarvish], [https://github.com/cuviper/dwarvish])

AM_INIT_AUTOMAKE([1.13 foreign subdir-objects])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([dw], [dwarf_next_unit], [],
             [AC_MSG_FAILURE([elfutils libdw >= 0.148 is required])])
AC_CHECK_LIB([elf], [elf_getshdrstrndx], [],
             [AC_MSG_FAILURE([elfutils libelf >= 0.142 is required])])

# NB: glib-2.32 is required for glib-compile-resources.
# That's around gtk+ 3.4, so that might as well be the baseline.
AM_PATH_GLIB_2_0([2.32.0], [], [AC_MSG_FAILURE([glib >= 2.32 is required])], [gmodule])
AM_PATH_GTK_3_0([3.4.0], [], [AC_MSG_FAILURE([gtk+ >= 3.4 is required])])

# Checks for header files.
AC_CHECK_HEADER([dwarf.h], [], [AC_MSG_FAILURE([dwarf.h is required])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT
