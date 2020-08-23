# sthkd - simple terminal hot key daemon
# See LICENSE file for copyright and license details.
.POSIX:

include config.mk

SRC = sthkd.c
OBJ = $(SRC:.c=.o)
DESTDIR=
PREFIX=/usr/local

all: options sthkd

options:
	@echo sthkd build options:
	@echo "CFLAGS  = $(STCFLAGS)"
	@echo "LDFLAGS = $(STLDFLAGS)"
	@echo "CC      = $(CC)"

config.h:
	cp config.def.h config.h

.SUFFIX: .o.c
.c.o:
	$(CC) $(ALLCFLAGS) -c $<

sthkd.o: sthkd.c

$(OBJ):

sthkd: $(OBJ)
	$(CC) -o $@ $(OBJ) $(ALLLDFLAGS)

clean:
	rm -f sthkd $(OBJ) sthkd-$(VERSION).tar.gz

dist: clean
	mkdir -p sthkd-$(VERSION)
	cp -R Makefile config.mk sthkd.c $(SRC) sthkd-$(VERSION)
	tar -cf - sthkd-$(VERSION) | gzip > sthkd-$(VERSION).tar.gz
	rm -rf sthkd-$(VERSION)

install: sthkd
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f sthkd $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/sthkd
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < sthkd.1 > $(DESTDIR)$(MANPREFIX)/man1/sthkd.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/sthkd.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/sthkd
	rm -f $(DESTDIR)$(MANPREFIX)/man1/sthkd.1

.PHONY: all options clean dist install uninstall
