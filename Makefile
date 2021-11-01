CC = gcc
CFLAGS = -lrt -lpthread -Werror -Wall -g
LDFLAGS = -lrt -lpthread

all: simulator management_system 
# new_firealarm new_firealarm2	

# new_firealarm: new_firealarm.o

# new_firealarm.o: new_firealarm.c


new_firealarm2: new_firealarm2.o

new_firealarm2.o: new_firealarm2.c# new_firealarm2.o: new_firealarm2.c

simulator: simulator.o

simulator.o: simulator.c

management_system: management_system.o

management_system.o: management_system.c

clean:
	rm -f sim *.o

.PHONY: all clean