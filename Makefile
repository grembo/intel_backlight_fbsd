CC ?= clang
CFLAGS +=
LDFLAGS = -ldrm_intel -lpciaccess

INCS = -I/usr/local/include -I/usr/local/include/libdrm
LIBS = -L/usr/local/lib

PREFIX ?= /usr/local
SRC = intel_reg_map.c intel_mmio.c intel_backlight.c intel_drm.c intel_pci.c

all: intel_backlight

intel_backlight: $(SRC)
	$(CC) -o intel_backlight $(INCS) $(LIBS) $(SRC) $(CFLAGS) $(LDFLAGS)
	strip intel_backlight

install: intel_backlight
	install -m4555 intel_backlight "$(DESTDIR)$(PREFIX)/bin"

clean:
	rm -f intel_backlight
