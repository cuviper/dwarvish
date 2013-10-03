AC_PREREQ([2.69])
AC_INIT([Dwarvish], [0.1], [https://github.com/cuviper/dwarvish/issues],
        [dwarvish], [https://github.com/cuviper/dwarvish])

AM_INIT_AUTOMAKE([1.13 foreign])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([dw], [dwfl_begin], [], [AC_MSG_FAILURE([elfutils libdw is required])])
AM_PATH_GLIB_2_0([2.0.0], [], [AC_MSG_FAILURE([glib 2.0+ is required])])
AM_PATH_GTK_3_0([3.0.0], [], [AC_MSG_FAILURE([gtk 3.0+ is required])])

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
