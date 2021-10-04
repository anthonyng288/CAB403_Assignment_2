#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

//status of the LPR sensors 
//and keep track of where each car is in the car park



//tell the boomgates when to open and when to close



//control what is displayed on the information signs at each entrance
void entranceDisplay (int entranceNo) {

}


//Ensure there is room in the car park before allowing new vehicles in
int findAvailableSpace () {
    //need to lock the space so if there is only 1 space left and
    //any new cars come along, they wont be allocated the same space 
    
    //return the level that the car needs to park on
    return 0;
}


//keep track of how long each car has been in the parking
//and produce a bill once the car leaves
void timeSpent (int timeEntered, int timeExited){
    //    
}



//display the current status of the parking lot on a frequently
//updating screen, showing how full each level is, the current
//status of the boom gates, signs, temperature sensors and alarms, 
//as well as the revenue the car park has brought in so far
void displayScreen () {
    //get the status of each level and display how full the level is
    printf("----------------------------------------\n");
    printf("~Level 1~\n");
    printf("Capacity: %d/20\n", 2);
    
    printf("~Level 2~\n");
    printf("Capacity: %d/20\n", 12);
    
    printf("~Level 3~\n");
    printf("Capacity: %d/20\n", 4);
}

int main() {
    displayScreen();
    return 0;
}
