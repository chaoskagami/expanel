SRCDIR := src
BUILDDIR := .mk/build

# included from .mk/config.mk
# CFLAGS :=
LIBS :=

# included from $(SRCDIR)/Makefile
TARGETS :=

# define in $(SRCDIR)/Makefile
DEPS :=

CC := gcc
LD := gcc
CFLAGS := -std=gnu99

all: setup srcs

clean:
	@echo -e "CLEAN\t\texpanel"
	@rm -rf expanel
	@echo -e "CLEAN\t\t$(BUILDDIR)"
	@rm -R $(BUILDDIR)

setup:
	@mkdir -p $(BUILDDIR) $(patsubst %,$(BUILDDIR)/%,$(SRCDIR))

.mk/config.mk:
	./configure

.PHONY: all setup srcs

-include .mk/config.mk
-include $(patsubst %,%/Makefile,$(SRCDIR))
-include $(DEPS)

install:
	@echo -e "INSTALL\t\tbmpanel -> $(DESTDIR)$(PREFIX)/bin"
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@cp -f bmpanel $(DESTDIR)$(PREFIX)/bin
	@chmod 755 $(DESTDIR)$(PREFIX)/bin/bmpanel
	@echo -e "INSTALL\t\tthemes -> $(DESTDIR)$(PREFIX)/share/expanel"
	@mkdir -p $(DESTDIR)$(PREFIX)/share/expanel
	@cp -R themes $(DESTDIR)$(PREFIX)/share/expanel

srcs: $(TARGETS)
