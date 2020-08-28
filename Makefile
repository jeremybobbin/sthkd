# itty - simple terminal hot key daemon
# See LICENSE file for copyright and license details.
.POSIX:

include config.mk

SRC = itty.c
OBJ = $(SRC:.c=.o)
DESTDIR=
PREFIX=/usr/local

all: options itty

options:
	@echo itty build options:
	@echo "CFLAGS  = $(STCFLAGS)"
	@echo "LDFLAGS = $(STLDFLAGS)"
	@echo "CC      = $(CC)"

config.h:
	cp config.def.h config.h

.SUFFIX: .o.c
.c.o:
	$(CC) $(ALLCFLAGS) -c $<

itty.o: itty.c

$(OBJ):

itty: $(OBJ)
	$(CC) -o $@ $(OBJ) $(ALLLDFLAGS)

clean:
	rm -f itty $(OBJ) itty-$(VERSION).tar.gz

dist: clean
	mkdir -p itty-$(VERSION)
	cp -R Makefile config.mk itty.c $(SRC) itty-$(VERSION)
	tar -cf - itty-$(VERSION) | gzip > itty-$(VERSION).tar.gz
	rm -rf itty-$(VERSION)

install: itty
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f itty $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/itty
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < itty.1 > $(DESTDIR)$(MANPREFIX)/man1/itty.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/itty.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/itty
	rm -f $(DESTDIR)$(MANPREFIX)/man1/itty.1

.PHONY: all options clean dist install uninstall
