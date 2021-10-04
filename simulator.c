#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define LEVELS 5
#define ENTRANCES 5
#define EXITS 5
//Simulate car
typedef struct Cars {
    const char* rego;
    int level;
    int timeEntered;

}Car;
typedef struct Levels {
    int space;

}Level;
typedef enum boomGateState {
    opened, closed, rising, lowering
} bg_state;
typedef enum { false, true } bool;

void simulate() {
    bool space;
    Level level1; 
    level1.space = 12;
    Car car1;
    car1.rego = "123456";
    bg_state boom1;
    space = true;
    //read rego
    //dertermine if they can enter or not
    while(space == true) {
        boom1 = closed;
        switch(boom1) {
            case closed: printf("The boom gate is closed.\n") ;
        }
        
        if(car1.rego == "123456") { //check if the number plate matches one in the list
            printf("The car is allowed access.\n");

            //is there any space available?
            if(level1.space < 20) {
                space = true;
                //open boom gate
                boom1 = opened;
                printf("There is space for this car.\n");
            }
            else {
                space = false;
                printf("There is no space for this car.\n");
            }
        }
    
        //allocate the space to the car
        if(space == true) {
            car1.level = 1;
            level1.space = level1.space + 1;
            };
        //add car to level's list of cars
        
        printf("Level 1's space filled in now: %d\n", level1.space);
        switch(boom1) {
            case opened: printf("The boom gate is closed.\n") ;
        }
    }
    
    //
}

//Simulate boom gates


//Simulate temperature
int main() {
    simulate();
    return 0;
}


