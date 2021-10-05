#pragma once
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>


#define SHM_SIZE 2920
#define SHM_NAME "PARKING"

//
// Third level data structs
//

// Data struct for License Plate Recognition sensor
typedef struct pc_lpr {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char l_plate[6];
    char padding[2];

} pc_lpr_t;

// Data struct for Boom Gate
typedef struct pc_boom {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char status;
    char padding[7];

} pc_boom_t;

// Data struct for Information Sign
typedef struct pc_sign {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char display;
    char padding[7];

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
    pc_lpr_t lpt;
    pc_boom_t boom;

} p_exit_t;

// Data struct for one level
typedef struct p_level
{
    pc_lpr_t lpr;
    int16_t temp;
    u_int8_t alarm;
    char padding[5];


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



typedef struct shared_memory {
    /// The name of the shared memory object.
    const char* name;

    /// The file descriptor used to manage the shared memory object.
    int fd;

    /// Address of the shared data block. 
    shared_data_t *data;
} shared_memory_t;

// Create a shared memory segment
// returns: true if successful, false if failed
bool create_shared_object( shared_memory_t* shm, const char* share_name ) {

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

    if ((shm->data = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm->fd, 0)) == MAP_FAILED){
        return false;
    }

    return true;


}

bool get_shared_object( shared_memory_t* shm, const char* share_name ) {
    
    if ((shm->fd = shm_open(share_name, O_RDWR, 0666)) < 0)
    {
        shm->data = NULL;
        return false;
    }
}

