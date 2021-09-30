#pragma once
#include <semaphore.h>


typedef struct shared_data {
    /// Used by controller to notify worker when a task is available.
    //sem_t controller_semaphore;

    /// Used by worker to notify controller when a result is available.
    //sem_t worker_semaphore;

    /// Operation assigned by controller, telling worker what to do. 
    //operation_t operation;

    /// Parameters for the operation, assigned by controller and read by worker.
    double lhs, rhs;

    /// Result of operation, assigned by worker and read by controller.
    double result;
} shared_data_t;

typedef struct shared_memory {
    /// The name of the shared memory object.
    const char* name;

    /// The file descriptor used to manage the shared memory object.
    int fd;

    /// Address of the shared data block. 
    shared_data_t* data;
} shared_memory_t;