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

#include "shm.h"
#include "boomgate.h"


#define PLATES_FILE "Plates.txt"
#define HASHTABLE_BUCKETS 100


// Global variables
shared_mem_t shm;
int levels_fullness[LEVELS];

// Get shared memory segment
bool get_shared_object( shared_mem_t* shm, const char* share_name ){
    
    if ((shm->fd = shm_open(share_name, O_RDWR, 0666)) < 0) {
        shm->data = NULL;
        return false;
    }
    return true;
}



////////////////////////////////
////        Hashtable       ////
/////////////////////////////////

typedef struct item item_t;
struct item
{
    u_int8_t *value;
    item_t *next;
};

// A hash table mapping a string to an integer.
typedef struct htab htab_t;
struct htab
{
    item_t **buckets;
    size_t size;
};


// Init hash table
bool htab_init(htab_t *h, size_t n)
{
    
    h->buckets = (item_t **)calloc(n, sizeof(item_t *));
    h->size = n;

    return h->buckets != NULL;
}

// The Bernstein hash function.
// Modified to only use 6 character strings (reliabillity)
size_t djb_hash(char *s)
{
    // ensure string formats are standardized
    int input[7];
    for (size_t i = 0; i < 6; i++)
    {
        input[i] = s[i];
    }
    size_t hash = 5381;
    for (size_t i = 0; i < 6; i++)
    {
        // hash = hash * 33 + c
        hash = ((hash << 5) + hash) + input[i];
    }
    return hash;
}

// Calculate the offset for the bucket for key in hash table.
size_t htab_index(htab_t *h, char *key)
{
    return djb_hash(key) % h->size;
}

// Convert the key into an array of ints
// perams: key must be a string of 6 characters
u_int8_t* get_val(char* key){
    u_int8_t *temp = calloc(6, sizeof(u_int8_t));
    for (int i = 0; i < 6; i++)
    {
        temp[i] = (u_int8_t)key[i];
    }
    return temp;
}

// Add a value into hash table
bool htab_add(htab_t *h, char *key){
    // get pointer to head of bucket list
    //item_t *head = h->buckets[(djb_hash(key) % h->size)];

    item_t *new_node = (item_t *)malloc(sizeof(item_t));
    if (new_node == NULL){
        return false;
    }

    new_node->value = get_val(key);
    new_node->next = h->buckets[htab_index(h, key)];
    h->buckets[htab_index(h, key)] = new_node;

    return true;
}


// Read through a file of numberplates and insert into hash table
// conditions:
//     input: a text file consisting of lines of exactly 6 characters
void read_plates(const char *input, htab_t *h){
    FILE* text  = fopen(input, "r");
    char line[10];
    
    while (NULL != fgets(line, sizeof(line), text))
    {
        htab_add(h, line);
    }
}

// Recursive function to search a bucket in a has table for a value
// conditions:
//     input: must be at least 6 characters
//     input: only first 6 characters will be checked        
bool search_bucket(item_t *item, u_char *input){
    for (size_t i = 0; i < 6; i++)
    {
        if (input[i] != item->value[i])
        {
            if (item->next == NULL)
            {
                return false;
            } else{
                // search next bucket
                return search_bucket(item->next, input);
            }
        }
    }
    // search successfull
    return true;
}

// Determine if an input is stored in the hash table
// condidions:
//  input: must be at least 6 characters
//  input: only first 6 characters will be checked
bool search_plate(htab_t *h, u_char *input){
    // determine bucket result would be stored in
    item_t *head = h->buckets[(djb_hash((char*)input) % h->size)];
    if (head == NULL)
    {
        return false;
    } else{
        return search_bucket(head, input);
    }
}



//Ensures that there is room in the car park before
//allowing new vehicles in (number of cars < number of levels * the number of cars per level).

bool cp_has_space (int num_cars){

    if num_cars < (LEVELS * LEVEL_CAPACITY){
        return true;
    }
    else{
        return false;
    }
}



//Chooses a random available level
char available_level(){
    
    //show a character between ‘1’ and ‘5’ 
    //floor the driver should park on

    for (int i = 0; i < LEVELS; i++){
        if (levels_fullness[i] < LEVEL_CAPACITY){
            levels_fullness[i] += 1;
            char c = (i + 1) +'0';
            return c;           
        }
    }

    // If the driver is unable to access the car park due to it being full, the sign will show the
    //character ‘F
    return 'F';

}


//Determine what message to input in the entry sign for the car
// conditions:
// input: if the plate is or not allowed in
// input: if the carpark has space or not
char entry_message( bool search_plate, bool cp_has_space){
    
   //
    if(cp_has_space){
        if (search_plate){
            //show a character between ‘1’ and ‘5’ 
            //floor the driver should park on
            return available_level();
        }
        else{
            //If the driver is unable to access the car park due to not being in the access file, the
            //sign will show the character ‘X’
            ;
            return 'X';
        }
    }
    else{
        // If the driver is unable to access the car park due to it being full, the sign will show the
        //character ‘F
        return 'F';
    }
}





int main(){

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


    // Close shared memory
    munmap((void *)shm.data, SHM_SIZE);
    close(shm.fd);

    return 0;
}   

