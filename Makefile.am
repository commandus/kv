SUBDIRS = .
ACLOCAL_AMFLAGS = -I m4
commoncppflags = -Ithird_party

if OS_MACH
LIB_LMDB=lmdb
OS_CPPFLAGS=-DUSE_LMDB
else
# uncomment to use mdbx instead of lmdb
# OS_CPPFLAGS=
# LIB_LMDB=mdbx

# Dynamic library
# LIB_LMDB=-l lmdb
# Static library
#LIB_LMDB=/usr/lib/x86_64-linux-gnu/liblmdb.a -lpthread
# Dynamic library
LIB_LMDB=-llmdb -lpthread
OS_CPPFLAGS=-DUSE_LMDB
endif

commoncppflags += $(OS_CPPFLAGS)

#
# Binaries
#
bin_PROGRAMS = kv

nobase_dist_include_HEADERS = \
	platform.h \
	third_party/argtable3/argtable3.h \
	errlist.h log.h \
	kv-config.h kv-params.h

commonlibs = -L/usr/local/lib $(LIB_LMDB)

kv_SOURCES = \
	kv.cpp errlist.cpp kv-params.cpp kv-config.cpp \
	dbop.cpp \
	config-filename.cpp \
	third_party/argtable3/argtable3.c

kv_LDADD = $(commonlibs)
kv_CPPFLAGS = $(commoncppflags) 

#
# Configs, readme, CMake etc.
#
configdir = $(datadir)
dist_config_DATA = \
  CMakeLists.txt CODE_OF_CONDUCT.md CONTRIBUTING.md COPYING HISTORY LICENSE README.md TODO
