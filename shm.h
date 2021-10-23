#pragma once
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>



//
// Third level data structs
//

// Data struct for License Plate Recognition sensor
typedef struct pc_lpr {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char l_plate[6];

} pc_lpr_t;

// Data struct for Boom Gate
typedef struct pc_boom {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char status;

} pc_boom_t;

// Data struct for Information Sign
typedef struct pc_sign {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char display;

} pc_sign_t;

//
// Second level data structs
//

// Data struct for one level
typedef struct p_enterance {
    pc_lpr_t lpr;
    pc_boom_t boom;
    pc_sign_t sign;

} p_enterance_t;

// Data struct for one exit
typedef struct p_exit
{
    pc_lpr_t lpr;
    pc_boom_t boom;

} p_exit_t;

// Data struct for one level
typedef struct p_level
{
    pc_lpr_t lpr;
    int16_t temp;
    u_int8_t alarm;
    //char padding[5];


} p_level_t;

//
// First level data struct
//

// Data used by shared memory
typedef struct shared_data {
    p_enterance_t enterances[5];
    p_exit_t exits[5];
    p_level_t levels[5];

} shared_data_t;

typedef struct shared_mem {
    /// The name of the shared memory object.
    const char* name;

    /// The file descriptor used to manage the shared memory object.
    int fd;

    /// Address of the shared data block. 
    shared_data_t *data;
} shared_mem_t;
