CC = gcc
CFLAGS = -lrt -lpthread -Werror -Wall -g
LDFLAGS = -lrt -lpthread

all: simulator management_system

simulator: simulator.o

simulator.o: simulator.c

management_system: management_system.o

management_system.o: management_system.c

clean:
	rm -f sim *.o

.PHONY: all clean