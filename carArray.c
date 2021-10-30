#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#include "defines.h"

typedef struct car {
    char* lp;
    time_t enter_time;
    int park_level;
} car_t;

car_t *car_array[LEVELS][LEVEL_CAPACITY];

bool remove_car (char* license_plate) {
    for(int j = 0; j < LEVELS; j++) {
        for (int i = 0; i < LEVEL_CAPACITY; i++) {
            if(car_array[j][i] != NULL) {
                if(strcmp(license_plate, car_array[j][i]->lp) == 0) {
                    car_array[j][i] = NULL;
                    printf("\nFound the car in the array!");
                    return true;
                }
            }
        }
    }
    printf("Couldn't find the car!");
    return false;
}
bool new_car (car_t* car) {
    for(int i = 0; i < LEVEL_CAPACITY; i++){
        if(car_array[car->park_level-1][i] == NULL) {
            car_array[car->park_level-1][i] = car;
            return true;
        }
    }
    return false;
}


int main () {

    car_t* car = malloc(sizeof(*car));
    car->lp = "888888";
    car->park_level = 3;
    new_car(car);

    car_t* car2 = malloc(sizeof(*car2));
    car2->lp = "999999";
    car2->park_level = 1;
    new_car(car2);

    car_t* car3 = malloc(sizeof(*car3));
    car3->lp = "121212";
    car3->park_level = 1;
    // new_car(car3);

    remove_car(car3->lp);
    // remove_car("555555");

    for (int i = 0; i < LEVELS; i++) {
        printf("\nLevel %d", i+1);
        for (int j = 0; j < LEVEL_CAPACITY; j++) {
            if(car_array[i][j] == NULL) {
                printf("\nNULL");
            }
            else {
                printf("\nLP: %s Level: %d", car_array[i][j]->lp, car_array[i][j]->park_level );
            }
        }
        printf("\n");
    }
    printf("\nSuccess?");


    free(car);
    free(car2);
    free(car3);


    return 0;

}