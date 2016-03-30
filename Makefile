all: xmake tags

xmake: dep-dag.o simple-makefile.o build.o xmake.o
	cc -o xmake dep-dag.o simple-makefile.o build.o xmake.o

dep-dag.o: dep-dag.c dep-dag.h
	cc -Wall -Wextra -std=c99 -pedantic -O -c dep-dag.c

simple-makefile.o: simple-makefile.c dep-dag.h xmake.h
	cc -Wall -Wextra -std=c99 -pedantic -O -D_POSIX_C_SOURCE=200112L -c simple-makefile.c

build.o: build.c dep-dag.h xmake.h
	cc -Wall -Wextra -std=c99 -pedantic -O -D_POSIX_C_SOURCE=200112L -c build.c

xmake.o: xmake.c dep-dag.h xmake.h
	cc -Wall -Wextra -std=c99 -pedantic -O -c xmake.c

tags: build.c dep-dag.c dep-dag.h simple-makefile.c xmake.c xmake.h
	ctags -R .
