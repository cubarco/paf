CC = gcc
CFLAGS += -Wall

paf: paf.o
paf.o: src/paf.c
	${CC} ${CFLAGS} -c $^

.PHONY: all clean
all: paf
clean:
	rm paf *.o
