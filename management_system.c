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
#include <string.h>

#include "shm.h"
#include "defines.h"
#include "status_display.c"
#include "rand_temp.c"





// Global variables
shared_mem_t shm; // shared memory
bool exit_condition = false; // exit condition
int parking_capacity;
volatile int car_count = 0;
pthread_mutex_t capacity_lock;
int levels_fullness[LEVELS];
double revenue;


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
    pthread_t entrance_threads[ENTRANCES];
    pthread_t lpr_threads[ENTRANCES]; // only entrance implemented
    pthread_t status_display;
    pthread_t rand_temps;
} thread_list_t;


////////////////////////////////
////           Car          ////
////////////////////////////////

typedef struct car {
    char* lp;
    time_t enter_time;
} car_t;

// Calculates the bill for a car when they trigger the exit LPR
void bill_car(car_t* car){
    // Open billing.txt in append mode
    FILE* fp = fopen(BILLING_FILE, "a");
    // Calculate the difference between car->enter_time 
    // and exit_time
    time_t exit_time = time(0) * 1000;
    double total_time = difftime(exit_time, car->enter_time);
    // Calculate total bill from time_spent in the car park
    double bill = total_time * 0.05;
    
    // Write to billing.txt file
    fprintf(fp,"%s $%.02f\n", car->lp, bill);
    fclose(fp);
}
// Calculates the total revenue for the car park
void calculate_total_revenue() {
    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    // double revenue = 0.0;
    
    if((fp = fopen(BILLING_FILE, "r")) != NULL) {
        while ((read = getline(&line, &len, fp)) != -1) {
            char money_str[len];
            double money_dbl;
            char *eptr;
            // Get substring of each car's bill total
            strncpy(money_str, line+8, len);
            // Convert string to double
            money_dbl = strtod(money_str, &eptr);
            revenue += money_dbl; 
        }  
        fclose(fp);
    }
    else {
        printf("File does not exist.");
    }    
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
            if (item->next != NULL) {
                return search_bucket(item->next, input);
            } else {
                return false;
            }
            
            
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
////////////////////////////////
////       Entering         ////
////////////////////////////////
//Ensures that there is room in the car park before
//allowing new vehicles in (number of cars < number of levels * the number of cars per level).
bool cp_has_space (int num_cars){
    if (num_cars < (LEVELS * LEVEL_CAPACITY)){
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

struct var_entrance_manager {
    p_enterance_t* ent;
    htab_t* h;
};

void entrance_lpr(var_entrance_manager_t* variables) {
    p_enterance_t* ent = variables->ent;
    htab_t* h = variables->h;
    char send_to_level;
    pthread_mutex_lock(&ent->lpr.lock);

    while (1)
    {
        while (ent->lpr.l_plate[0] == '\0')
        {
            pthread_cond_wait(&ent->lpr.cond, &ent->lpr.lock);
        }
        
        if (search_plate(h, (char*)ent->lpr.l_plate)){
            pthread_mutex_lock(&capacity_lock);
            if (car_count < parking_capacity){
                car_count++;
                printf("CURRENT CAP %d\n", car_count);
                send_to_level = available_level();
                pthread_mutex_unlock(&capacity_lock);
                pthread_mutex_lock(&ent->sign.lock);
                ent->sign.display = send_to_level;
                pthread_mutex_unlock(&ent->sign.lock);

            } else {
                pthread_mutex_unlock(&capacity_lock);
                pthread_mutex_lock(&ent->sign.lock);
                ent->sign.display = 'X';
                pthread_mutex_unlock(&ent->sign.lock);
            }
        } else {
            pthread_mutex_lock(&ent->sign.lock);
            ent->sign.display = 'X';
            pthread_mutex_unlock(&ent->sign.lock);
        }
        printf("%c\n", send_to_level);
        ent->lpr.l_plate[0] = '\0'; // reset lpr
        pthread_cond_signal(&ent->sign.cond);
    }
    pthread_mutex_unlock(&ent->lpr.lock);
}

// Functions waits until a boomgate is signaled then opens or closes it
void manager_boomgate(pc_boom_t* boom){
    while (!exit_condition) {
        sleep(1);
    }
    
}

// Function updates the status_screen every 1sec
void status_screen(shared_mem_t* shm){
    while (!exit_condition) {
        status_display(levels_fullness, revenue, shm);
        sleep(1);
    }
}

// Function updates the temperature every 1-5secs
void rand_temp_thread(shared_mem_t* shm){
    while (!exit_condition) {
        rand_temp(shm);
        int secs = rand() % 6;
        sleep(secs);
    }
}

////////////////////////////////
////       Boomgates        ////
////////////////////////////////
//Tell when to raise boomgates
void boomgate_func_raising(pc_boom_t boomgate_protocol){
        pthread_mutex_lock(&boomgate_protocol.lock);
        if(boomgate_protocol.status == 'C'){
            // change the status automatically no waiting
            boomgate_protocol.status = 'R';
            pthread_cond_signal(&boomgate_protocol.cond);
        }
        pthread_mutex_unlock(&boomgate_protocol.lock);
        
      
}
//Tell when to lower boomgates
void boomgate_func_lowering(pc_boom_t boomgate_protocol){
    pthread_mutex_lock(&boomgate_protocol.lock);
        if(boomgate_protocol.status == 'O'){
            // change the status automatically no waiting
            boomgate_protocol.status = 'L';
            pthread_cond_signal(&boomgate_protocol.cond);
        }
        pthread_mutex_unlock(&boomgate_protocol.lock);
}
// Takes the time required (millisecons)
// and multiplies it (in case we want to make it slower for testing)
void sleeping_beauty(int seconds){
    usleep(seconds * MULTIPLIER);
}

////////////////////////////////
////        Threads         ////
////////////////////////////////

struct thread_var {
    var_entrance_manager_t entrance_vars[ENTRANCES];
};

bool init_threads(thread_list_t* t_list, thread_var_t* t_var, htab_t* htab){
    // Setup boomgates
    for (size_t i = 0; i < ENTRANCES; i++){
        t_var->entrance_vars[i].ent = &shm.data->entrances[i];
        t_var->entrance_vars[i].h = htab;
        if (pthread_create(&t_list->entrance_threads[i], NULL, (void*)entrance_lpr,
        &t_var->entrance_vars[i])){
            return EXIT_FAILURE;
        }
    }


    // entrance setup


    /*if(pthread_create(&t_list->status_display, NULL, 
    (void*)status_screen, &shm)){
        return EXIT_FAILURE;
    }*/
    if(pthread_create(&t_list->rand_temps, NULL, 
    (void*)rand_temp_thread, &shm)){
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}




int main(){
    // Access shared memory
    if (!get_shared_object(&shm, SHM_NAME))
    {
        printf("Memory access failed\n");
    }
    
    parking_capacity = LEVEL_CAPACITY * LEVELS;
    printf("CAP: %d\n", parking_capacity);

    // Setup for hash table and insert file contents
    htab_t hasht;
    size_t buckets = HASHTABLE_BUCKETS;
    if (!htab_init(&hasht, buckets)) {
        printf("Hashtable creation failed");
    }
    
    // init mutex for carpark capacity
    pthread_mutex_init(&capacity_lock, NULL);
    parking_capacity = LEVEL_CAPACITY * LEVELS;

    // setup all threads
    thread_list_t threads;
    thread_var_t thread_vars;
    if (init_threads(&threads, &thread_vars, &hasht)){
        printf("Thread creation failed");
    }

    //car_t temp = {"aaaaaa"};


    FILE* fp = fopen(BILLING_FILE, "w+");
    fclose(fp);

    //watch_lpr(&shm.data->entrances[0].lpr, &hasht);


    
    // Wait until program closes
    printf("Press ENTER to close the manager\n");
    getchar();
 
    // Memory cleanup
    //cleanup_threads(&threads);
    htab_destroy(&hasht);
    munmap((void *)shm.data, sizeof(shm.data));
    close(shm.fd);

    return 0;
}   
