TARGETS=ap
MANS=ap.1
CFLAGS=-O2 -g -Wall
INSTALL_BIN?=install -s

DOCBOOK2XMAN="docbook2x-man"

all: $(TARGETS) $(MANS)

clean:
	rm -f $(BINS) $(MANS)

install:
	mkdir -p $(DESTDIR)/usr/bin
	$(INSTALL_BIN) $(TARGETS) $(DESTDIR)/usr/bin
	
	mkdir -p $(DESTDIR)/usr/share/man/man1
	install $(MANS) $(DESTDIR)/usr/share/man/man1

ap.1: ap.docbook
	$(DOCBOOK2XMAN) $<
%.1: %
	pod2man --center=" " $< > $@;
