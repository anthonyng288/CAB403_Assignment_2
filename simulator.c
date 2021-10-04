pragma once
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


pthread_mutex_t cp_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cp_condvar = PTHREAD_COND_INITIALIZER;

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

int random_parking_time(){
    //lock mutex
    pthread_mutex_lock(&pr->mutex_pr);
    int parking_time = rand() 10000+100;
    pthread_mutex_unlock(&pr->mutex_pr);
    
    return parking_time;

}
