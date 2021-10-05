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


#define PLATES_FILE "Plates.txt"
#define HASHTABLE_BUCKETS 100

// Global variables
shared_memory_t shm;




void main(){

    // Access shared memory
    if (get_shared_object(&shm, SHM_NAME))
    {
        printf("Memory access successful\n");
    } else{
        printf("Memory access failed\n");
    }

    // Setup for hash table and insert file contents
    htab_t hasht;
    size_t buckets = HASHTABLE_BUCKETS;
    htab_init(&hasht, buckets);

    // For testing, print hash table and contents
    htab_print(&hasht);
    //htab_check(&hasht);

    // For testing, exit check
    
    
}

