dnl Process this file with autoconf to produce a configure script.
AC_INIT([dj64dev-host], [0.1])
AC_CONFIG_FILES([
  Makefile.conf
  dj64host.pc
])

VERSION="$PACKAGE_VERSION"
AC_SUBST(VERSION)

AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_HEADERS([dosemu2/emu.h],, AC_MSG_ERROR([dosemu2 headers missins]))
CFLAGS="$CFLAGS -Wall"

AC_OUTPUT
