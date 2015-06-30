# Process this file with autoconf to produce a configure script.
#
# xfstt configure.ac
#

AC_INIT([xfstt], [1.9.1])
AC_PREREQ([2.53])
AC_CONFIG_SRCDIR([src/xfstt.cc])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])

AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.11 -Wall foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_ARG_WITH([xcb],
            [AS_HELP_STRING([--with-xcb], [enable tools using XCB])],
            [],
            [with_xcb=no])
AM_CONDITIONAL(WITH_XCB, [test "x$with_xcb" != "xno"])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])
AS_IF([test "x$with_xcb" != "xno"],
      [PKG_CHECK_MODULES(XCB, [xproto xcb xcb-keysyms xcb-image])])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS([fcntl.h netinet/in.h sys/socket.h sys/time.h unistd.h])

AC_CHECK_DECL([PF_INET6],
	      [AC_DEFINE([HAVE_IPV6], [1],
			 [The system has IPv6 support.])],
	      [], [#include <sys/socket.h>])
AC_CHECK_DECL([IPV6_V6ONLY],
	      [AC_DEFINE([HAVE_IPV6_V6ONLY], [1],
			 [The system has IPv6 v6 only support.])],
	      [], [#include <netinet/in.h>])

PKG_CHECK_MODULES(FONTSPROTO, [fontsproto])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_STAT
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([gettimeofday pathconf munmap mkdir rmdir select socket])

# Checks for i18n
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18.2])

AC_CONFIG_FILES([
	Makefile
	doc/Makefile
	po/Makefile.in
	libfstt/Makefile
	src/Makefile
])
AC_OUTPUT
