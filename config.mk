# sxhkd version
VERSION = 0.0.0

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

PKG_CONFIG = pkg-config

# includes and libs
INCS =
LIBS = -lutil

# flags
ALL_CPPFLAGS = -DVERSION=\"$(VERSION)\" -D_XOPEN_SOURCE=600
ALL_CFLAGS = $(INCS) $(CPPFLAGS) $(CFLAGS)
ALL_LDFLAGS = $(LIBS) $(LDFLAGS)

CC = c99
