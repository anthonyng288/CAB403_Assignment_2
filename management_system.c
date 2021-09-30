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

#include "hashtable.h"
#include "shm.h"

#define SHM_SIZE 1440
#define PLATES_FILE "Plates.txt"
#define HASHTABLE_BUCKETS 100


bool create_shared_object( shared_memory_t* shm, const char* share_name ) {
    // Remove any previous instance of the shared memory object, if it exists.
    shm_unlink(shm->name);

    // Assign share name to shm->name.
    shm->name = share_name;

    // Create the shared memory object, allowing read-write access, and saving the
    // resulting file descriptor in shm->fd. If creation failed, ensure 
    // that shm->data is NULL and return false.
    if ((shm->fd =shm_open(shm->name, O_CREAT | O_RDWR, 0666)) < 0)
    {
        shm->data = NULL;
        return false;
    }
    

    // Set the capacity of the shared memory object via ftruncate. If the 
    // operation fails, ensure that shm->data is NULL and return false. 
    if (ftruncate(shm->fd, SHM_SIZE) != 0){
        shm->data = NULL;
        return false;
    }

    // Otherwise, attempt to map the shared memory via mmap, and save the address
    // in shm->data. If mapping fails, return false.
    if ((shm->data = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm->fd, 0)) == MAP_FAILED){
        return false;
    }

    // shared memory creation successful
    return true;
}

void main(){

    // Setup for hash table and insert file contents
    htab_t hasht;
    size_t buckets = HASHTABLE_BUCKETS;
    htab_init(&hasht, buckets);
    read_plates(PLATES_FILE, &hasht);

    // For testing, print hash table and contents
    htab_print(&hasht);
    htab_check(&hasht);

}

