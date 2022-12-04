#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT(libpwent, 0.1, libpwent@copblock.app)
AC_CONFIG_SRCDIR([pwent.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_CHECK_TOOL(AR,ar)
dnl Set default for ARFLAGS, since autoconf does not have a macro for it.
dnl This allows people to set it when running configure or make
test -n "$ARFLAGS" || ARFLAGS="cr"
AC_SUBST(ARFLAGS)
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h netdb.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([endgrent endpwent memset])

AC_CONFIG_FILES([Makefile Makefile.defs])
AC_OUTPUT
