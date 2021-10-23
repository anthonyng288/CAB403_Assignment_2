#pragma once
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>


#include "shm.h"



// boomgate function for simulator
// conditions:
//  must be run before partner function in manager
//  partner function is required to run properly
//  boomgate must start shut
void manager_boomgate(pc_boom_t* boom){
    struct timespec ms20req, ms20rem = {0, 20000000}; // 20 ms
    struct pc_boom *boomgate = boom;
    
    pthread_mutex_lock(&boomgate->lock);

    // run until thread is killed
    while (1) { 
        pthread_cond_wait(&boomgate->cond, &boomgate->lock);
        if (boomgate->status == 'C') {
            boomgate->status = 'R';
        } else {
            printf("ERROR: boomgate could not be opened");
        }
        pthread_cond_wait(&boomgate->cond, &boomgate->lock); // wait for gate to be opened
        nanosleep(&ms20req, &ms20rem); // boomgate is locket open for 20 ms
        if (boomgate->status == 'O') {
            boomgate->status = 'L';
        } else {
            printf("ERROR: boomgate could not be shut");
        }
    }
}

// open a boomgate to allow a car to pass through
// conditions:
//  input: boomgate.status must be 'C'
void sim_boomgate_open(pc_boom_t boomgate){
    struct timespec ms10req, ms10rem = {0, 10000000}; // 10 ms
    

    pthread_cond_signal(&boomgate.cond); // boomgate status sets to 'R'
    nanosleep(&ms10req, &ms10req); // wait for gate to open
    pthread_mutex_lock(&boomgate.lock);
    if (boomgate.status != 'R') {
        printf("ERROR: boomgate  didn't open");
    }
    boomgate.status = 'O'; // boomgate is open
    pthread_cond_signal(&boomgate.cond); // boomgate status sets to 'L'
    nanosleep(&ms10req, &ms10rem); // wait for gate to close
    if (boomgate.status != 'L') {
        printf("ERROR: boomgate  didn't close");
    }
    boomgate.status = 'C';
}