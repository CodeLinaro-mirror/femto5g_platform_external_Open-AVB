OBJS=emac
INCL=emac.h
AVBLIB=libemac.a
#CFLAGS=-ggdb

CC?=gcc
RANLIB?=ranlib

all: $(AVBLIB)

doc: build/Makefile
	$(MAKE) -s -C build doc
	@echo "\n\nTo display documentation use:\n\n" \
		"\txdg-open $(abspath build/documents/api_docs/index.html)\n"

$(AVBLIB): $(addsuffix .o,$(OBJS))
	$(RM) $@
	$(AR) rcs $@ $^
	$(RANLIB) $@

emac.o: emac.c $(INCL)
	$(CC) -c $(INCFLAGS) $(CFLAGS) -I../neutrino/ emac.c

app:
	gcc -static main.c -L. -lemac -o pps.out

clean:
	$(RM) `find . -name "*~" -o -name "*.[oa]" -o -name "\#*\#" -o -name TAGS -o -name core -o -name "*.orig" -o -name *.out -o -name *main`


