#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "shm.h"


// Global variables
shared_memory_t shm;


// Access a segment of shared memory
// returns: true if successfull, false if failed


int main()
{
    // Create shared memory segment
    if(!create_shared_object(&shm, SHM_NAME)){
        printf("Creation of shared memory failed\n");
    } else{
        printf("Creation of shared memory successful\n");
    }
    
    return 0;
}




