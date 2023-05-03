#MAKEFLAGS= -rR
include Makefile.defs
include Makefile.rules


all: libpwent.a

srcs:=$(wildcard *.c)
objs:=$(srcs:.c=.o)
cpps:=$(srcs:.c=.i)

$(w arning srcs=$(srcs))
$(w arning cpps=$(srcs:.c=.i))
$(w arning objs=$(srcs:.c=.o))

$(cpps): $(srcdir)/pwent.c $(srcdir)/config.h
	$(CC) $< -o $@ -E

$(objs): %.o: %.i
	$(CC) $< -o $@ -c

$(libdir)/libpwent.a: libpwent.a $(libdir)
	install $< $@

install: $(libdir)/libpwent.a

(%): %
	$(AR) $(ARFLAGS) $@ $< #x

libpwent.a: $(objs)
	ar cr $@ $<
	ranlib $@

Makefile.defs config.h: %: %.in
	./config.status

Makefile.rules.def:
	env - make -f /dev/null /dev/null -p > $@.new
	mv $@.new $@

clean:

.PHONY: all clean install

