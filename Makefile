PREFIX = /usr/local

src = $(wildcard src/*.c)
obj = $(src:.c=.o)
bin = shapeblobs

CFLAGS = -pedantic -Wall -g -O3 -ffast-math
LDFLAGS = -lGL -lGLU -lX11 -lXext -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: install
install: $(bin)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $(bin) $(DESTDIR)$(PREFIX)/bin/$(bin)

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(bin)
