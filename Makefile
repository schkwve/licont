LIBS := cap seccomp

CFLAGS := -Wall -Werror $(LIBS:%=-l%)

.PHONY: licont
licont: src/licont.c
	gcc $< $(CFLAGS) -o $@

clean:
	rm -f licont