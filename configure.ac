# -*- sh -*-
AC_INIT([libmlocknice], [0.1])
AC_CONFIG_SRCDIR([src/mlocknice.h])
AM_INIT_AUTOMAKE([-Wall])
AM_CONFIG_HEADER([config.h])
AC_LANG(C)
AC_GNU_SOURCE
AM_MAINTAINER_MODE

##################################################################
# Enable debug flag
##################################################################

AC_ARG_ENABLE([debug], 
              [AS_HELP_STRING([--enable-debug],
              [Compile with extra debug code (default=no)])], 
              [libmlocknice_use_debug=yes], [libmlocknice_use_debug=no])

WFLAGS="-Wall -fPIC"
if test x${libosso_use_debug} = xyes ; then
    LIBMLOCKNICE_CFLAGS="-g -D_GNU_SOURCE $WFLAGS"
else
    LIBMLOCKNICE_CFLAGS="-g -D_GNU_SOURCE -DNDEBUG $WFLAGS"
fi

AC_SUBST([LIBMLOCKNICE_CFLAGS])

##################################################################

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h errno.h stdio.h sys/mman.h])

# Checks for library functions.
AC_FUNC_STRERROR_R
AC_FUNC_VPRINTF
AC_FUNC_STAT

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT

echo "

$PACKAGE_NAME configuration:
	version: $PACKAGE_VERSION

Options:
	libmlocknice cflags:........ $LIBMLOCKNICE_CFLAGS
	Include debug code:.... $libmlocknice_use_debug
"
