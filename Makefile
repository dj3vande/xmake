xmake: dep-dag.o setup-dag.o build.o xmake.o
	cc -o xmake dep-dag.o setup-dag.o build.o xmake.o

dep-dag.o: dep-dag.c dep-dag.h
	cc -Wall -Wextra -std=c99 -pedantic -O -c dep-dag.c

setup-dag.o: setup-dag.c dep-dag.h xmake.h
	cc -Wall -Wextra -std=c99 -pedantic -O -D_POSIX_C_SOURCE=200112L -c setup-dag.c

build.o: build.c dep-dag.h xmake.h
	cc -Wall -Wextra -std=c99 -pedantic -O -D_POSIX_C_SOURCE=200112L -c build.c

xmake.o: xmake.c dep-dag.h xmake.h
	cc -Wall -Wextra -std=c99 -pedantic -O -c xmake.c
