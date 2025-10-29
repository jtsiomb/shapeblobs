PREFIX = /usr/local

src = src/main_x11.c src/blobs.c src/msurf2.c src/image.c src/timer.c src/dynarr.c
obj = $(src:.c=.o)
bin = shapeblobs

CFLAGS = -O3
LIBS = -lGL -lGLU -lX11 -lXext -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS) $(LIBS)

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

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
