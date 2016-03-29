all: dep-dag.o setup-dag.o

dep-dag.o: dep-dag.c dep-dag.h
	cc -Wall -Wextra -std=c99 -pedantic -O -c dep-dag.c

setup-dag.o: setup-dag.c dep-dag.h xmake.h
	cc -Wall -Wextra -std=c99 -pedantic -O D_POSIX_C_SOURCE=200112L -c setup-dag.c
