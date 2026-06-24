CFLAGS = -Wall -O2

build: main.c
	gcc $(CFLAGS) main.c -o main

clean: main
	rm -f main

debug: main.c
	gcc $(CFLAGS) -g main.c -o main
