#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


int shm_fd;
volatile void *shm;
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t alarm_condvar = PTHREAD_COND_INITIALIZER;

#define LEVELS 5
#define ENTRANCES 5
#define EXITS 5

typedef struct license_plate_recognition_sensor{

    pthread_mutex_t lock;
	pthread_cond_t condition_variable;
    char license_plate;

} LPR;

typedef struct park_boom_gates{

    pthread_mutex_t lock;
	pthread_cond_t condition_variable;
    char status;

} boom_gate;

typedef struct information_sign{

    pthread_mutex_t lock;
	pthread_cond_t condition_variable;
    char info;

} info_sign;

typedef struct temperature_sensors{

    int16_t current_temperature;

} temp_sensor;

typedef struct fire_alarms{

    char status;

} fire_alarm;

typedef struct park_entrance{
    
    LPR lpr_EN; 
    boom_gate boom_gate_EN;
    info_sign sign;
    
} entrance_s;

typedef struct park_exit{  

    LPR lpr_EX;
    boom_gate boom_gate_EX;

} exit_s;



typedef struct park_levels{

    LPR lpr_LVL;
    temp_sensor ts;
    fire_alarm alarm;
    
} level;

/**
 * A shared data block.
 */
typedef struct shared_memory{

    entrance_s ent;
    exit_s ext;
    level lvl;

} shared_memory_t;

//Protect calls to rand() with a mutex as rand () accesses a global variable
//containing the current random seed.)

typedef struct protect_rand{
    pthread_mutex_t mutex_pr;

} protected_rand;


int random_parking_time(){
    //lock mutex
    pthread_mutex_lock(&pr->mutex_pr);
    int parking_time = rand() 10000+100;
    pthread_mutex_unlock(&pr->mutex_pr);
    
    return parking_time;

}

int random_car_creation_time(){
    //lock mutex
    pthread_mutex_lock(&pr->mutex_pr);
    int creation_time = rand() 100+1;
    pthread_mutex_unlock(&pr->mutex_pr);
    
    return creation_time;

}

int random_license_plate(protected_rand* pr){
    //lock mutex
    pthread_mutex_lock(&pr->mutex_pr);
    //Randomise if the car will have a valid license plate from hash table or a random plate
    //Random plate could be one of the list, but it is unlikely
    int valid = rand() % 2;
    

    char plate[];
    //char digits[10] = {"0123456789"};
    
    //if the car plate is random
    //License plates from the hash table are 3 numbers then 3
    //letters so we will asuume that format
    if valid == 0{
        //random plate

        for( i = 0 ; i < 3 ; i++ ) {
        strncat(plate, rand() % 10, 1);
        }

        for( i = 0 ; i < 3 ; i++ ) {
        strncat(plate,'A' + (rand() % 26, 1);
        }

    }

    //if from the hash table
    else {
        //plate from hash table
        plate = 0;
    }
    pthread_mutex_unlock(&pr->mutex_pr);
    return plate;
}