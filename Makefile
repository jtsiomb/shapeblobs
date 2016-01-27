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
