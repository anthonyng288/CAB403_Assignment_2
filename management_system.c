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
shared_mem_t shm; // shared memory
bool exit_condition = false; // exit condition
int parking_capacity;
volatile int car_count = 0;


typedef struct var_entrance_manager var_entrance_manager_t;
typedef struct thread_var thread_var_t;


// Get shared memory segment
bool get_shared_object( shared_mem_t* shm, const char* share_name ){
    
    if ((shm->fd = shm_open(share_name, O_RDWR, 0666)) < 0) {
        shm->data = NULL;
        return false;
    } else if ((shm->data =  mmap(NULL, 2920, PROT_READ | PROT_WRITE,
        MAP_SHARED, shm->fd, 0)) < 0) {
        return false;
    }
    return true;
}

typedef struct thread_list {
    pthread_t boomgate_threads[ENTRANCES + EXITS];
    pthread_t lpr_threads[ENTRANCES]; // only entrance implemented
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
struct item {
    char value[6];
    item_t *next;
};

// A hash table mapping a string to an integer.
typedef struct htab htab_t;
struct htab
{
    item_t **buckets;
    size_t size;
};


// The Bernstein hash function.
// Modified to only use 6 character strings (reliabillity)
size_t djb_hash(char s[6])
{
    size_t hash = 5381;
    for (size_t i = 0; i < 6; i++)
    {
        // hash = hash * 33 + c
        hash = ((hash << 5) + hash) + s[i];
    }
    return hash;
}

// Calculate the offset for the bucket for key in hash table.
size_t htab_index(htab_t *h, char *key)
{
    char temp[6];
    for (int i = 0; i < 6; i++)
    {
        temp[i] = key[i];
    }
    
    return djb_hash(temp) % h->size;
}


// Add a value into hash table
bool htab_add(htab_t *h, char *key){
    // get pointer to head of bucket list
    //item_t *head = h->buckets[(djb_hash(key) % h->size)];

    item_t *new_node = (item_t *)malloc(sizeof(item_t));
    if (new_node == NULL){
        return false;
    }

    for (int i = 0; i < 6; i++) {
        new_node->value[i] = key[i];
    }
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

// Init hash table
bool htab_init(htab_t *h, size_t n)
{
    h->size = n;
    h->buckets = malloc(n * sizeof(item_t*));
    if (h->buckets == NULL) {
        return false;
    }
    read_plates(PLATES_FILE, h);
    return true;
    
}

// Recursive function to search a bucket in a has table for a value
// conditions:
//     input: must be at least 6 characters
//     input: only first 6 characters will be checked        
bool search_bucket(item_t *item, char *input){
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

// DEBUGGING
void print_htab_bucket(item_t* bucket){
    for (int i = 0; i < 6; i++)
    {
        printf("%c", bucket->value[i]);
    }
    printf(" -> ");
    if (bucket->next != NULL)
    {
        print_htab_bucket(bucket->next);
    }
    
    
}

// DEBUGGING
void print_htab(htab_t* h) {
    for (int i = 0; i < h->size; i++) {
        printf("Bucket %d: ", i);
        if (h->buckets[i] == NULL)
        {
            printf(" EMPTY");
        } else {
            print_htab_bucket(h->buckets[i]);
        }
        printf("\n");
    }   
}

// Determine if an input is stored in the hash table
// condidions:
//  input: must be at least 6 characters
//  input: only first 6 characters will be checked
bool search_plate(htab_t *h, char *input){
    // determine bucket result would be stored in
    item_t *head = h->buckets[(djb_hash((char*)input) % h->size)];
    if (head == NULL)
    {
        return false;
    } else{
        return search_bucket(head, input);
    }
}

struct var_entrance_manager {
    p_enterance_t* ent;
    htab_t* h;
};

void entrance_lpr(var_entrance_manager_t* variables) {
    p_enterance_t* ent = variables->ent;
    htab_t* h = variables->h;
    //pthread_cond_wait(&ent->lpr.cond, &ent->lpr.lock);
    printf("%c\n", ent->sign.display);
    if (search_plate(h, (char*)&ent->sign.display)) {
        printf("ACCEPT\n");
    } else {
        printf("DECLINE");
    }
}



// Functions waits until a boomgate is signaled then opens or closes it
void manager_boomgate(pc_boom_t* boom){
    while (!exit_condition) {
        sleep(1);
    }
    
}

////////////////////////////////
////        Threads         ////
////////////////////////////////






struct thread_var {
    var_entrance_manager_t lpr_entrance_vars[ENTRANCES];
};



bool init_threads(thread_list_t* t_list, thread_var_t* t_var, htab_t* htab){
    // Setup boomgates
    for (size_t i = 0; i < ENTRANCES; i++){
        if (pthread_create(&t_list->boomgate_threads[i], NULL, (void*)manager_boomgate,
        &shm.data->entrances[i].boom)){
            return EXIT_FAILURE;
        }
    }
    for (size_t i = 0; i < EXITS; i++){
        if (pthread_create(&t_list->boomgate_threads[i + ENTRANCES], NULL, 
        (void*)manager_boomgate, &shm.data->exits[i].boom)){
            return EXIT_FAILURE;
        }
    }
    // Setup entrance LPRs
    /*for (size_t i = 0; i < ENTRANCES; i++) {
        t_var->lpr_entrance_vars[i].h = htab;
        t_var->lpr_entrance_vars[i].ent = &shm.data->entrances[i];
        if (pthread_create(&t_list->lpr_threads[i], NULL, (void*)entrance_lpr,
        &t_var->lpr_entrance_vars[i]));
    }

    if (pthread_create(&t_list->boomgate_threads[0], NULL, (void*)manager_boomgate,
    &shm.data->entrances[0].boom)){
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
    // Enterances
    for (size_t i = 0; i < ENTRANCES; i++){
        pthread_join(t_list->boomgate_threads[i], NULL);
    }
    for (size_t i = 0; i < EXITS; i++){
        pthread_join(t_list->boomgate_threads[i + EXITS], NULL);
    }
    // LPRs
    /*for (size_t i = 0; i < ENTRANCES; i++) {
        pthread_cond_broadcast(&shm.data->entrances[i].lpr.cond);
        pthread_join(t_list->lpr_threads[i], NULL);
    }*/
    
}



int main(){
    // Access shared memory
    if (!get_shared_object(&shm, SHM_NAME))
    {
        printf("Memory access failed\n");
    }
    printf("%d\n", shm.data->temp);
    /*
    parking_capacity = LEVEL_CAPACITY * LEVELS;

    // Setup for hash table and insert file contents
    htab_t hasht;
    size_t buckets = HASHTABLE_BUCKETS;
    if (!htab_init(&hasht, buckets)) {
        printf("Hashtable creation failed");
    }
    
    

    // setup all threads
    thread_list_t threads;
    thread_var_t thread_vars;
    if (init_threads(&threads, &thread_vars, &hasht)){
        printf("Thread creation failed");
    }*/

    //car_t temp = {"aaaaaa"};

    ////FILE* fp = fopen(BILLING_FILE, "w+");
    //fprintf(fp,"sssssss\n");
    ////fclose(fp);

    //watch_lpr(&shm.data->entrances[0].lpr, &hasht);

    // Wait until program closes
    ////printf("Press ENTER to close the manager\n");
    ////getchar();
 

    // Memory cleanup
    ////cleanup_threads(&threads);
    ////htab_destroy(&hasht);
    munmap((void *)shm.data, sizeof(shm.data));
    close(shm.fd);

    return 0;
}   
