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
ALLCPPFLAGS = -DVERSION=\"$(VERSION)\" -D_XOPEN_SOURCE=600
ALLCFLAGS = $(INCS) $(CPPFLAGS) $(CFLAGS)
ALLLDFLAGS = $(LIBS) $(LDFLAGS)

CC = c99
