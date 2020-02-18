SUBDIRS = .
ACLOCAL_AMFLAGS = -I m4
commoncppflags = -Ithird_party

if OS_MACH
LMDB=lmdb
OS_CPPFLAGS=-DUSE_LMDB
else
OS_CPPFLAGS=
LMDB=mdbx
endif

#
# Binaries
#
bin_PROGRAMS = kv

nobase_dist_include_HEADERS = \
	platform.h \
	third_party/argtable3/argtable3.h \
	errlist.h \
	kv-config.h

commonlibs = -L/usr/local/lib 

kv_SOURCES = \
	kv.cpp errlist.cpp \
	config-filename.cpp \
	third_party/argtable3/argtable3.c

kv_LDADD = $(commonlibs) -l
kv_CPPFLAGS = $(commoncppflags)

#
# Configs, readme, CMake etc.
#
configdir = $(datadir)
dist_config_DATA = \
  CMakeLists.txt CODE_OF_CONDUCT.md CONTRIBUTING.md COPYING HISTORY LICENSE README.md TODO
