MAKEFLAGS= -rR

include Makefile.defs

all: libpwent.a


pwent.i: $(srcdir)/pwent.c $(srcdir)/config.h
	$(CPP) $< -o $@ -E

pwent.o: pwent.i

libpwent.a: libpwent.a(pwent.o)

(%): %
	$(AR) $(ARFLAGS) $@ $<

$(libdir)/libpwent.a: $(libdir)
	install $< $@

install: $(libdir)/libpwent.a

Makefile.defs config.h: %: %.in
	./config.status

.PHONY: all
