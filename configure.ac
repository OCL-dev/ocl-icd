#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([OpenCL ICD Loader], [2.1.1], [ocl-icd-devel@lists.forge.imag.fr], [ocl-icd])
AC_CONFIG_SRCDIR([ocl_icd_loader.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign 1.9 tar-pax check-news color-tests parallel-tests])
AMX_SILENT_RULES([AM_DEFAULT_VERBOSITY=1])
# AM_PROG_AR must be called before LT_INIT...
AM_PROG_AR
LT_INIT([disable-static])
AC_CONFIG_TESTDIR([tests], [.])

OPENCL_VERSION="1.2"
AC_SUBST([OPENCL_VERSION])

# Checks for programs.
AC_PROG_CC_C89
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_CC_C_O

AC_PATH_PROG([ASCIIDOC], [asciidoc], [])
AC_PATH_PROG([A2X], [a2x], [])
AC_PATH_PROG([XMLTO], [xmlto], [])
AM_CONDITIONAL([GEN_DOC], [\
	test x"$ASCIIDOC" != x && \
	test x"$A2X" != x && \
	test x"$XMLTO" != x ])

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])
cl_h=no
case $host_os in
  darwin* )
    AC_CHECK_HEADER([OpenCL/opencl.h],[cl_h=yes])
    ;;
  * )
    AC_CHECK_HEADER([CL/opencl.h],[cl_h=yes])
    ;;
esac
if test $cl_h = no; then
  AC_MSG_ERROR([OpenCL headers are required])
fi
AC_MSG_CHECKING([whether OpenCL headers 1.2 are available])
AC_PREPROC_IFELSE([
  AC_LANG_PROGRAM([[
#if defined(__APPLE__) || defined(__MACOSX)
#  include <OpenCL/opencl.h>
#else
#  include <CL/opencl.h>
#endif
#ifndef CL_VERSION_1_2
#  error Cannot find CL_VERSION_1_2 symbol
#endif
    ]], [[
    ]])],[
  AC_MSG_RESULT([yes])
],[
  AC_MSG_RESULT([no])
  AC_MSG_FAILURE([OpenCL headers for OpenCL 1.2 are required])
])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_C_CONST

NO_DEPRECATED_DECLARATIONS_FLAGS=
AX_CHECK_COMPILER_FLAGS([-Wno-deprecated-declarations],
  [NO_DEPRECATED_DECLARATIONS_FLAGS="-Wno-deprecated-declarations"])
AC_SUBST([NO_DEPRECATED_DECLARATIONS_FLAGS])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([strstr])

# --enable-pthread-once
AC_ARG_ENABLE([pthread-once],
  [AS_HELP_STRING([--enable-pthread-once],
               [use pthread_once() to ensure correct initialisation without polling])],
  [pthread_once=$enableval],
  [pthread_once=no])

if test "$pthread_once" = yes ; then
  AX_PTHREAD([
    AC_DEFINE([USE_PTHREAD], [1],
              [pthreads are available])
  ], [
    AC_MSG_ERROR([No pthread detected on this platform!])
  ])
fi

# --disable-debug
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--disable-debug],
               [remove debug and additionnal checks (default is included)])],
  [use_debug=$enableval],
  [use_debug=yes])
if  test "x$use_debug" != xno ; then
  AC_DEFINE([DEBUG_OCL_ICD], [1],
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
  doc/Makefile
  tests/Makefile

  OpenCL.pc
  ocl-icd.pc
  tests/atlocal
])
AC_OUTPUT
