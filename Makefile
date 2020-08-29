# itty - simple terminal hot key daemon
# See LICENSE file for copyright and license details.
.POSIX:

include config.mk

SRC = itty.c bmac.c
OBJ = $(SRC:.c=.o)
DESTDIR=
PREFIX=/usr/local

all: options itty bmac

options:
	@echo itty build options:
	@echo "CFLAGS  = $(ALL_CFLAGS)"
	@echo "LDFLAGS = $(ALL_LDFLAGS)"
	@echo "CC      = $(CC)"

config.h:
	cp config.def.h config.h

.SUFFIX: .o.c
.c.o:
	$(CC) $(ALL_CFLAGS) -c $<

itty: itty.o
	$(CC) -o $@ $^ $(ALL_LDFLAGS)

bmac: bmac.o
	$(CC) -o $@ $^ $(ALL_LDFLAGS)

clean:
	rm -f itty bmac $(OBJ) itty-$(VERSION).tar.gz

dist: clean
	mkdir -p itty-$(VERSION)
	cp -R Makefile config.mk itty.c $(SRC) itty-$(VERSION)
	tar -cf - itty-$(VERSION) | gzip > itty-$(VERSION).tar.gz
	rm -rf itty-$(VERSION)

install: bmac itty
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f bmac itty sthkd $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/itty
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < itty.1 > $(DESTDIR)$(MANPREFIX)/man1/itty.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/itty.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/itty
	rm -f $(DESTDIR)$(MANPREFIX)/man1/itty.1

.PHONY: all options clean dist install uninstall
