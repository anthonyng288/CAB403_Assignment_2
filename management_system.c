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
#include <pthread.h>
#include <time.h>

#include "shm.h"
#include "defines.h"




// Global variables
shared_mem_t shm;
bool exit_condition = false; // exit condition

// Get shared memory segment
bool get_shared_object( shared_mem_t* shm, const char* share_name ){
    
    if ((shm->fd = shm_open(share_name, O_RDWR, 0)) < 0) {
        shm->data = NULL;
        return false;
    } else {
        shm->data =  mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, shm->fd, 0);
    }
    return true;
}

typedef struct thread_list {
    pthread_t boomgate_threads[ENTRANCES + EXITS];
} thread_list_t;


////////////////////////////////
////           Car          ////
////////////////////////////////

typedef struct car {
    char* lp;
    time_t enter_time;
} car_t;

void bill_car(car_t* car){
    FILE* fp = fopen(BILLING_FILE, "a");
    fprintf(fp,"sssssss\n");
    fclose(fp);
    return;
}


////////////////////////////////
////        Hashtable       ////
////////////////////////////////

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
    h->size = n;
    h->buckets = malloc(n * sizeof(item_t*));
    if (h->buckets == NULL) {
        return false;
    }
    return true;
    
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

// Destroy an initialised hash table.
// pre: htab_init(h)
// post: all memory for hash table is released
void htab_destroy(htab_t *h)
{
    for (int i = 0; i < h->size; i++){
        item_t* head = h->buckets[i];

        item_t* curr = head;
        item_t* next;
        while (curr != NULL)
        {
            next = curr->next;
            free(curr);
            curr = next;
        }
    }
    free(h->buckets);
    h->buckets = NULL;
    h->size = 0;
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
            return search_bucket(item->next, input);
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
    // determine bucket result would be sim_boomgate_openstored in
    item_t *head = h->buckets[(djb_hash((char*)input) % h->size)];
    if (head == NULL)
    {
        return/* code */ false;
    } else{
        return search_bucket(head, input);
    }
}



// Functions waits until a boomgate is signaled then opens or closes it
void manager_boomgate(pc_boom_t* boom){
    while (!exit_condition) {
        sleep(1);
    }
    
}

bool init_threads(thread_list_t* t_list){
    // Calculate number of boomgates
    for (size_t i = 0; i < ENTRANCES; i++){
        if (pthread_create(&t_list->boomgate_threads[i], NULL, (void*)manager_boomgate,
        &shm.data->enterances[i].boom)){
            return EXIT_FAILURE;
        }
    }
    for (size_t i = 0; i < EXITS; i++){
        if (pthread_create(&t_list->boomgate_threads[i + ENTRANCES], NULL, 
        (void*)manager_boomgate, &shm.data->exits[i].boom)){
            return EXIT_FAILURE;
        }
    }


    /*if (pthread_create(&t_list->boomgate_threads[0], NULL, (void*)manager_boomgate,
    &shm.data->enterances[0].boom)){
        return EXIT_FAILURE;
    }*/

    return EXIT_SUCCESS;
}

void exit_boomgates(thread_list_t* t_list){
    for (size_t i = 0; i < ENTRANCES; i++){
        pthread_join(t_list->boomgate_threads[i], NULL);
    }
    for (size_t i = 0; i < EXITS; i++){
        pthread_join(t_list->boomgate_threads[i + EXITS], NULL);
    }
}

void cleanup_threads(thread_list_t* t_list){
    exit_condition = true;
    exit_boomgates(t_list);
}






int main(){
    // Access shared memory
    if (!get_shared_object(&shm, SHM_NAME))
    {
        printf("Memory access failed\n");
    }

    // Setup for hash table and insert file contents
    htab_t hasht;
    size_t buckets = HASHTABLE_BUCKETS;
    htab_init(&hasht, buckets);

    // setup all thrreads
    thread_list_t threads;
    if (init_threads(&threads)){
        printf("Thread creation failed");
    }

    //car_t temp = {"aaaaaa"};

    FILE* fp = fopen(BILLING_FILE, "w+");
    //fprintf(fp,"sssssss\n");
    fclose(fp);

    // Wait until program closes
    printf("Press ENTER to close the manager\n");
    getchar();
    

    // Memory cleanup
    cleanup_threads(&threads);
    htab_destroy(&hasht);
    munmap((void *)shm.data, sizeof(shm.data));
    close(shm.fd);

    return 0;
}   
