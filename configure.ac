AC_PREREQ([2.69])
AC_INIT([dwarvish], [0.1])

AM_INIT_AUTOMAKE([1.13 foreign])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([dw], [dwfl_begin])
PKG_CHECK_MODULES([gtk3], [gtk+-3.0])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT
