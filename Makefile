MAKEFLAGS= -rR
include Makefile.defs


all: libpwent.a


pwent.i: $(srcdir)/pwent.c $(srcdir)/config.h
	$(CPP) $< -o $@ -E

$(libdir)/libpwent.a: libpwent.a $(libdir)
	install $< $@

install: $(libdir)/libpwent.a

Makefile.defs config.h: %: %.in
	./config.status

Makefile.rules.def:
	env - make -f /dev/null /dev/null -p > $@.new
	mv $@.new $@


.PHONY: all

include Makefile.rules

