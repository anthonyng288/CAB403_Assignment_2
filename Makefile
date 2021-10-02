CC = gcc
CFLAGS = -Werror -g -lpthread -lrt

all: carpark

carpark: simulator.o management_system.o

simulator.o: simulator.c

management_system.o: management_system.c

clean:
	rm -f carpark *.o

.PHONY: all clean