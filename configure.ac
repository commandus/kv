AC_PREREQ([2.63])
AC_INIT([kv], [0.1], [andrei.i.ivanov@commandus.com])
AM_INIT_AUTOMAKE([1.11 foreign no-define subdir-objects])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_SRCDIR([kv.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])
build_mac=no
# Detect the target system
case "${host_os}" in
  darwin*)
    build_mac=yes
    ;;
esac
AM_CONDITIONAL([OS_MACH], [test "$build_mac" = "yes"])

# MacOS: install openssl into /usr/local/lib/opt/openssl@1.1
# brew install openssl@1.1
LDFLAGS="$LDFLAGS -L/usr/local/lib"

# Checks for header files. Add /usr/local/include for OS X.
CFLAGS="$CFLAGS -I/usr/local/include"

AC_CHECK_HEADERS([lmdb.h], [], AC_MSG_ERROR([Could not find lmdb.h. Try $ ./configure CFLAGS='-Iyour-lmdb-include-path]))
AC_CHECK_LIB(lmdb, mdb_env_create, [], AC_MSG_ERROR([Could not find liblmdb. Try $ ./configure LDFLAGS='-Lyour-lmdb-lib-path']))

AC_OUTPUT
