#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([OpenCL ICD Loader], [1.1], [ocl-icd-devel@lists.forge.imag.fr], [ocl-icd])
AC_CONFIG_SRCDIR([ocl_icd_loader.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign 1.9 tar-pax check-news color-tests parallel-tests])
AMX_SILENT_RULES([AM_DEFAULT_VERBOSITY=0])
LT_INIT([disable-static])

# Checks for programs.
AC_PROG_CC_C89
AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([strstr])

# --disable-debug
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--disable-debug],
               [remove debug and additionnal checks (default is included)])],
  [use_debug=$enableval],
  [use_debug=yes])
if  test "x$use_debug" = xno ; then
  AC_DEFINE([DEBUG_OCL_ICD], [0],
            [Add DEBUG additionnal code])
fi

# --enable-update-database
AC_ARG_ENABLE([update-database],
  [AS_HELP_STRING([--enable-update-database],
               [update the YAML database from the currently installed ICD Loader and OpenCL headers (root access required)])],
  [update_database=$enableval],
  [update_database=no])
AM_CONDITIONAL([UPDATE_DATABASE], [test x"$update_database" != xno])

# always use versionned symbols (check required for MacOSX)
AM_CONDITIONAL([USE_MAP], [true])

AC_CONFIG_FILES([
  Makefile
  OpenCL.pc
])
AC_OUTPUT
