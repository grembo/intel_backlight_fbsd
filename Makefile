CC ?= clang
CFLAGS +=
LDFLAGS = -ldrm_intel -lpciaccess

INCS = -I/usr/local/include -I/usr/local/include/libdrm
LIBS = -L/usr/local/lib

PREFIX ?= /usr/local
MANPREFIX ?= $(PREFIX)
SRC = intel_reg_map.c intel_mmio.c intel_backlight.c intel_drm.c intel_pci.c

all: intel_backlight

intel_backlight: $(SRC)
	$(CC) -o intel_backlight $(INCS) $(LIBS) $(SRC) $(CFLAGS) $(LDFLAGS)
	strip intel_backlight

install: intel_backlight
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	install -m4555 intel_backlight "$(DESTDIR)$(PREFIX)/bin"
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man/man1"
	install -m0444 intel_backlight.1 "$(DESTDIR)$(MANPREFIX)/man/man1"

install-strip: install
	strip "$(DESTDIR)$(PREFIX)/bin/intel_backlight"

clean:
	rm -f intel_backlight
