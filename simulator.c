#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "shm.h"
#include "defines.h"
#include <semaphore.h>



// Global variables

int shm_fd;
// shared_mem_t sh_mem; // shared memory

int shm_fd;
volatile void *shm;
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t alarm_condvar = PTHREAD_COND_INITIALIZER;

// Takes the time required (millisecons)
// and multiplies it (in case we want to make it slower for testing)
void sleeping_beauty(int seconds){
    
    usleep(seconds * MULTIPLIER);
}


// Create a shared memory segment
// returns: true if successful, false if failed
bool create_shared_object( shared_mem_t* shm, const char* share_name ) {

    if (shm->name != NULL)
    {
        shm_unlink(shm->name);
    }
    shm->name = share_name;

    if ((shm->fd =shm_open(share_name, O_CREAT | O_RDWR, 0666)) < 0)
    {
        shm->data = NULL;
        return false;
    }

    if (ftruncate(shm->fd, 2920) < 0){
        shm->data = NULL;
        return false;
    }

    if ((shm->data = mmap(0, 2920, PROT_READ |PROT_WRITE, MAP_SHARED, shm->fd, 0)) == MAP_FAILED){
            return false;
        }
    return true;
}

// Linked list for cars queing to get in

typedef struct node node_t;
struct node
{
    char licence[6];
    node_t *next;
};

// List of ques for entry to the carpark
node_t* car_entry_que[ENTRANCES];

// add a node to the end of the list
node_t* l_list_add(node_t* head, char car[6]){
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    if ((new_node == NULL)) {
        printf("ERROR: CANNOT ADD TO LINKED LIST\n");
        return NULL;
    }

    new_node->next = NULL;
    for (int i = 0; i < 6; i++) {
        new_node->licence[i] = car[i];
    }
    

    if (head == NULL) {
        return new_node;
    }
    node_t* temp = head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = new_node;
    return head;
}

// remove a node from the start of the linked list
node_t* l_list_remove(node_t* head){
    if (head == NULL) {
        return NULL;
    }
    if (head->next == NULL) { // one entry in list
        free(head);
        return NULL;
    }
    node_t* temp = head->next;
    free(head);
    return temp;
}

// Clear a list and set head to NULL
node_t* l_list_clear(node_t* head) {
    if (head == NULL) {
        return NULL;
    }
    node_t* prev = head;
    node_t* curr = head->next;
    do {
        free(prev);
        prev = curr;
        curr = curr->next;
    } while (curr != NULL);
    return NULL;
}

void print_lp(char input[6]) {
    for (u_int8_t i = 0; i < 6; i++) {
        printf(" %c", input[i]);
    }
}

// DEBUGGING
void l_list_print(node_t* head){
    if (head == NULL) {
        printf ("LIST IS EMPTY\n");
        return;
    }
    node_t* temp;
    temp = head;
    printf("LIST:");
    print_lp(temp->licence);
    while (temp->next != NULL) {
        temp = temp->next;
        printf(" ->");
        print_lp(temp->licence);
    }
    printf("\n");     
}




// initialises the mutex and cond for a returns 1 if it fails
// conditions: component must be a pc component from shm.h
// I tried to be fancy with void pointers and casting :( didn't work


int init_lpr(pc_lpr_t *lpr ,pthread_mutexattr_t* m_atr, pthread_condattr_t* c_atr){
    lpr->l_plate[0]= '\0';
    if (pthread_mutex_init(&lpr->lock, m_atr) != 0){
        return 1;
    } else if (pthread_cond_init(&lpr->cond, c_atr) != 0){
        return 1;
    } else {
        return 0;
    }
}

int init_boomgate(pc_boom_t *boomgate,pthread_mutexattr_t* m_atr, pthread_condattr_t* c_atr){
    boomgate->status = 'C';
    if (pthread_mutex_init(&boomgate->lock, m_atr) != 0){
        return 1;
    } else if (pthread_cond_init(&boomgate->cond, c_atr) != 0){
        return 1;
    } else {
        return 0;
    }
}

int init_sign(pc_sign_t *sign, pthread_mutexattr_t* m_atr, pthread_condattr_t* c_atr){
    sign->display = '\0';
    if (pthread_mutex_init(&sign->lock, m_atr) != 0){
        return 1;
    } else if (pthread_cond_init(&sign->cond, c_atr) != 0){
        return 1;
    } else {
        return 0;
    }
}

bool init_all(shared_data_t* data){
    int failed = 0;
    pthread_mutexattr_t mutex_atr;
    pthread_condattr_t cond_atr;

    pthread_mutexattr_init(&mutex_atr);
    pthread_mutexattr_setpshared(&mutex_atr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_init(&cond_atr);
    pthread_condattr_setpshared(&cond_atr, PTHREAD_PROCESS_SHARED);

    for (size_t i = 0; i < ENTRANCES; i++) {
        // entrances
        failed += init_lpr(&data->entrances[i].lpr, &mutex_atr, &cond_atr);
        failed += init_boomgate(&data->entrances[i].boom, &mutex_atr, &cond_atr);
        failed += init_sign(&data->entrances[i].sign, &mutex_atr, &cond_atr);
        car_entry_que[i] = NULL; // car que
    }
    for (u_int8_t i = 0; i < EXITS; i++){
        // exits
        failed += init_lpr(&data->exits[i].lpr, &mutex_atr, &cond_atr);
        failed += init_boomgate(&data->exits[i].boom, &mutex_atr, &cond_atr);
    }
    for (u_int8_t i = 0; i < LEVELS; i++) {
        // levels
        failed += init_lpr(&data->levels->lpr, &mutex_atr, &cond_atr);
        data->levels[i].alarm = 0;
    }
    if (failed == 0) {
        return true;
    } else {
        return false;
    }
}

//Protect calls to rand() with a mutex as rand () accesses a global variable
//containing the current random seed.)


typedef struct protect_rand{
    pthread_mutex_t lock;

} protect_rand_t;

////////////////////////////////
////       Boomgates        ////
///////////////////////////////

int random_parking_time(protect_rand_t pr){
    //lock mutex
    pthread_mutex_lock(&pr.lock);
    int parking_time = rand() % 10000+100;
    pthread_mutex_unlock(&pr.lock);
    
    return parking_time;
}

int random_car_creation_time(protect_rand_t pr){
    //lock mutex
    pthread_mutex_lock(&pr.lock);
    int creation_time = rand() % 100+1;
    pthread_mutex_unlock(&pr.lock);
    
    return creation_time;
}

//if the car plate is random
//License plates from the hash table are 3 numbers then 3
//letters so we will asume that format
char * unauthorised_plate(protect_rand_t pr){
    
    char *plate = malloc (6);
    
    
     // create 3 numbers
        for(int i = 0 ; i < 3 ; i++ ) {
        pthread_mutex_lock(&pr.lock);
        int num = rand() % 10;
        pthread_mutex_unlock(&pr.lock);
        
        //printf("%c \n", (num + '0'));
        
        // convert it to a char
        char num_c = (num + '0');
        //strncat(plate, &num_c, 1);
        plate[i] = num_c;
        //printf("%s \n\n", plate);
        
    // create 3 letters
    
        for(int i = 3 ; i < 6 ; i++ ) {
        pthread_mutex_lock(&pr.lock);
        char letters = 'A' + (rand() % 26);
        pthread_mutex_unlock(&pr.lock);
        plate[i] = letters;
        //printf("%s \n\n", plate);
        
        }
    
    }
    //printf("%s \n\n", "uwu");
    return plate;
}


// Random authorised car
// Assume that there is no repeat cars
/*char* authorised_plate(htab_t *h, protect_rand_t pr){
   
    // choose random bucket number
    pthread_mutex_lock(&pr.lock);
    int num = rand() % h->size;
    pthread_mutex_unlock(&pr.lock);

    // get plate from such bucket
    item_t random_item = h->buckets[num];

    char* plate =  malloc(6);
    plate = random_item ->value;

}

// Random license plate authorised or unauthorised
char* random_license_plate(htab_t *h, protect_rand_t pr){
    //lock mutex
    pthread_mutex_lock(&pr.lock);
    //Randomise if the car will have a valid license plate from hash table or a random plate
    //Random plate could be one of the list, but it is unlikely
    int valid = rand() % 2;
    
    pthread_mutex_unlock(&pr.lock);

    char* plate = NULL;
    //char digits[10] = {"0123456789"};
    
    
    if (valid == 0){
        plate = unauthorised_plate(pr);
    }
    //if from the hash table
    else {
        //plate from hash table
        plate = authorised_plate(h, pr);
    }
    return plate;
}*/

////////////////////////////////
////       Boomgates        ////
///////////////////////////////

//Tell when to open boomgates
void boomgate_func_open(pc_boom_t boomgate_protocol){
        pthread_mutex_lock(&boomgate_protocol.lock);
        if(boomgate_protocol.status == 'R'){
            // change the status to "O" after 10 milli
            sleeping_beauty(10);
            boomgate_protocol.status = 'O';
        }
        pthread_mutex_unlock(&boomgate_protocol.lock);

          
}

//Tell when to close boomgates
void boomgate_func_close(pc_boom_t boomgate_protocol){
    pthread_mutex_lock(&boomgate_protocol.lock);

        if(boomgate_protocol.status == 'L'){
            // change the status to "O" after 10 milli
            sleeping_beauty(10);
            boomgate_protocol.status = 'C';
        }
        pthread_mutex_unlock(&boomgate_protocol.lock);

}

// add a car the the que for an entrance and trigger LPR if needed
// if cars already exist within the que, lpr is already triggered and the list will be cleared
void car_add(shared_data_t* data, char licence[6], int entry) {
    pthread_cond_broadcast(&data->entrances[entry].lpr.cond);
}

typedef struct car_enty_struct car_entry_struct_t;
struct car_enty_struct {
    shared_data_t* data;
    int entry;
};


// ensures all cars enter the carpark WIP
void car_entry(car_entry_struct_t* input) {
    shared_data_t* data = input->data;
    int entry = input->entry;
    pthread_mutex_lock(&data->entrances[entry].sign.lock);
    while (data->entrances[entry].sign.display == '\0') {
        pthread_cond_wait(&data->entrances[entry].sign.cond, &data->entrances[entry].sign.lock);
    }
    if (data->entrances[entry].sign.display != 'X') {
        // create an instance of the car that has been admitted
    }
    l_list_remove(car_entry_que[entry]); // remove car from linked list
    printf("CAR STATUS %c\n", data->entrances[entry].sign.display);
    data->entrances[entry].sign.display = '\0'; // reset display
    if (car_entry_que[entry] != NULL) {
        usleep(2000);
        for (u_int8_t i = 5; i >= 0; i--) {
            data->entrances[entry].lpr.l_plate[i] = car_entry_que[entry]->licence[i];
        }
        pthread_cond_broadcast(&data->entrances[entry].lpr.cond);
    }
    
    
}

int main()
{
    shared_mem_t sh_mem; // shared memory

    // Create shared memory segment
    if(!create_shared_object(&sh_mem, SHM_NAME)){
        printf("Creation of shared memory failed\n");
    }
    if (!init_all(sh_mem.data)) {
        printf("Initialization failed\n");
    }
    // Testing random license generator
    // protect_rand_t pr= PTHREAD_MUTEX_INITIALIZER;
    // for (int i = 0; i < 10; i++){
    //     char *ram;
    //     ram = random_license_plate(pr);
    //     printf("%s \n", ram);
    // }




    sh_mem.data->temp = 66;
    printf("%d\n", sh_mem.data->temp);


    getchar();
    
    return EXIT_SUCCESS;
}