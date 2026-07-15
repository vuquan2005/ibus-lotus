CC?=cc
SHELL?=sh
PREFIX?=/usr

engine_name=lotus
engine_gui_name=ibus-setup-Lotus.desktop
ibus_e_name=ibus-engine-$(engine_name)
pkg_name=ibus-$(engine_name)
version=1.0.0

all: build

build:
	VERSION=$(version) $(SHELL) scripts/build

test:
	$(SHELL) scripts/test

clean:
	rm -rf bin dist
	rm -f *_linux *_cover.html go_test_* go_build_* test *.gz test

install: build
	VERSION=$(version) $(SHELL) scripts/install ${PREFIX} ${DESTDIR}

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/share/$(pkg_name)
	rm -rf $(DESTDIR)$(PREFIX)/lib/ibus-$(engine_name)/
	rm -f $(DESTDIR)$(PREFIX)/share/ibus/component/$(engine_name).xml
	rm -rf $(DESTDIR)$(PREFIX)/share/applications/$(engine_gui_name)

.PHONY: all build test clean install uninstall
