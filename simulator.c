#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "shm.h"
#include "defines.h"
#include <semaphore.h>



// Global variables
// shared_mem_t sh_mem; // shared memory

int shm_fd;
void *shm;
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t alarm_condvar = PTHREAD_COND_INITIALIZER;

// Create a shared memory segment
// returns: true if successful, false if failed
bool create_shared_object( shared_mem_t* shm, const char* share_name ) {

    if (shm->name != NULL)
    {
        shm_unlink(shm->name);
    }
    
    
    shm->name = share_name;
    if ((shm->fd =shm_open(shm->name, O_CREAT | O_RDWR, 0666)) < 0)
    {
        shm->data = NULL;
        return false;
    }

    if (ftruncate(shm->fd, SHM_SIZE) != 0){
        shm->data = NULL;
        return false;
    }

    if ((shm->data = (void *) mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm->fd, 0)) == MAP_FAILED){
        return false;
    }

    return true;


}


// initialises the mutex and cond for a returns 1 if it fails
// conditions: component must be a pc component from shm.h
// I tried to be fancy with void pointers and casting :( didn't work


int init_lpr(pc_lpr_t *lpr ,pthread_mutexattr_t* m_atr, pthread_condattr_t* c_atr){
    if (pthread_mutex_init(&lpr->lock, m_atr) != 0){
        return 1;
    } else if (pthread_cond_init(&lpr->cond, c_atr) != 0){
        return 1;
    } else {
        return 0;
    }
}

int init_boomgate(pc_boom_t *boomgate,pthread_mutexattr_t* m_atr, pthread_condattr_t* c_atr){
    boomgate->status = 'C';
    if (pthread_mutex_init(&boomgate->lock, m_atr) != 0){
        return 1;
    } else if (pthread_cond_init(&boomgate->cond, c_atr) != 0){
        return 1;
    } else {
        return 0;
    }
}

int init_sign(pc_sign_t *sign, pthread_mutexattr_t* m_atr, pthread_condattr_t* c_atr){
    if (pthread_mutex_init(&sign->lock, m_atr) != 0){
        return 1;
    } else if (pthread_cond_init(&sign->cond, c_atr) != 0){
        return 1;
    } else {
        return 0;
    }
}

bool init_mem(shared_data_t* data){
    int failed = 0;
    pthread_mutexattr_t mutex_atr;
    pthread_condattr_t cond_atr;

    pthread_mutexattr_init(&mutex_atr);
    pthread_mutexattr_setpshared(&mutex_atr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_init(&cond_atr);
    pthread_condattr_setpshared(&cond_atr, PTHREAD_PROCESS_SHARED);

    for (size_t i = 0; i < 5; i++) {
        // enterances
        failed += init_lpr(&data->enterances[i].lpr, &mutex_atr, &cond_atr);
        failed += init_boomgate(&data->enterances[i].boom, &mutex_atr, &cond_atr);
        failed += init_sign(&data->enterances[i].sign, &mutex_atr, &cond_atr);
        
        // exits
        failed += init_lpr(&data->exits[i].lpr, &mutex_atr, &cond_atr);
        failed += init_boomgate(&data->exits[i].boom, &mutex_atr, &cond_atr);

        // levels
        failed += init_lpr(&data->levels->lpr, &mutex_atr, &cond_atr);
        data->levels[i].alarm = 0;
    }
    if (failed == 0) {
        return true;
    } else {
        return false;
    }
}

//Protect calls to rand() with a mutex as rand () accesses a global variable
//containing the current random seed.)

typedef struct protect_rand{
    pthread_mutex_t mutex_pr;

} protected_rand;

// There were a few errors preventing compilation, commented out for now
/*
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
}*/


////////////////////////////////
////       Boomgates        ////
///////////////////////////////

//Tell when to open boomgates
void boomgate_func_open(pc_boom_t boomgate_protocol){
        pthread_mutex_lock(boomgate_protocol->lock);
        if(boomgate_protocol->status == 'R'){
            // change the status to "O" after 10 milli
            sleeping_beauty(10);
            boomgate_protocol->status = 'O';
        }
        pthread_mutex_unlock(boomgate_protocol->lock);
        
    }    


//Tell when to close boomgates
void boomgate_func_close(pc_boom_t boomgate_protocol){
    pthread_mutex_lock(boomgate_protocol->lock);
        if(boomgate_protocol->status == 'L'){
            // change the status to "O" after 10 milli
            sleeping_beauty(10);
            boomgate_protocol->status = 'C';
        }
        pthread_mutex_unlock(boomgate_protocol->lock);
}

// Takes the time required (millisecons)
// and multiplies it (in case we want to make it slower for testing)
void sleeping_beauty(int seconds){
    usleep(seconds * MULTIPLIER);
}

int main()
{
    shared_mem_t sh_mem; // shared memory

    // Create shared memory segment
    if(!create_shared_object(&sh_mem, SHM_NAME)){
        printf("Creation of shared memory failed\n");
    }
    if (!init_mem((shared_data_t*) sh_mem.data)) {
        printf("Initialization of shared memory failed\n");
    }
    
    // Testing
    printf("Press ENTER to end the simulation\n");
    getchar();
    

    return 0;
}