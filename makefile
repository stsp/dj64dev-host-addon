-include Makefile.conf

HOSTCRT0 = loader.o
ODIR = $(libdir)/dj64-host
MKDIR = $(datadir)/dj64-extras

all: Makefile.conf loader.o dj64host.pc

.PHONY: clean
clean:
	$(RM) *.pc *.o

Makefile.conf config.status: Makefile.conf.in configure
	./configure

configure: configure.ac
	autoreconf -v -i

$(HOSTCRT0): loader.c Makefile.conf

%.pc: %.pc.in config.status
	./config.status

install:
	$(INSTALL) -d $(DESTDIR)$(ODIR)
	$(INSTALL) -m 0644 $(HOSTCRT0) $(DESTDIR)$(ODIR)
	$(INSTALL) -d $(DESTDIR)$(datadir)/pkgconfig
	$(INSTALL) -m 0644 dj64host.pc $(DESTDIR)$(datadir)/pkgconfig
	$(INSTALL) -d $(DESTDIR)$(MKDIR)
	$(INSTALL) -m 0644 dj64host.mk $(DESTDIR)$(MKDIR)

uninstall:
	$(RM) $(DESTDIR)$(datadir)/pkgconfig/dj64host.pc
	$(RM) -r $(DESTDIR)$(MKDIR)
	$(RM) -r $(DESTDIR)$(ODIR)

rpm:
	make clean
	rpkg local && $(MAKE) clean >/dev/null

deb:
	debuild -i -us -uc -b && $(MAKE) clean >/dev/null
