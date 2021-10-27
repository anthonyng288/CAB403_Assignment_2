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
volatile void *shm;
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t alarm_condvar = PTHREAD_COND_INITIALIZER;





// Create a shared memory segment
// returns: true if successful, false if failed
bool create_shared_object( shared_mem_t* shm, const char* share_name ) {

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

    if (ftruncate(shm->fd, sizeof(shared_data_t)) != 0){
        shm->data = NULL;
        return false;
    }

    if ((shm->data = mmap(0, sizeof(shared_data_t), PROT_WRITE, MAP_SHARED, shm->fd, 0)) == MAP_FAILED){
        return false;
    }
    return true;
}

// Linked list for cars queing to get in
typedef struct car {
    // car struct
    char temp[6];
} car_t;

typedef struct node node_t;

// a node in a linked list of people
struct node
{
    car_t *car;
    node_t *next;
};

// List of ques for entry to the carpark
node_t* car_entry_que[ENTRANCES];

// add a node to the end of the list
node_t* l_list_add(node_t* head, car_t* car){
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    if ((new_node == NULL)) {
        printf("ERROR: CANNOT ADD TO LINKED LIST\n");
        return NULL;
    }

    new_node->next = NULL;
    new_node->car = car;

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

// get the first car in the que
car_t* l_list_get(node_t* head){
    if (head == NULL) {
        return NULL;
    } else {
        return head->car;
    }
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
    print_lp(temp->car->temp);
    while (temp->next != NULL) {
        temp = temp->next;
        printf(" ->");
        print_lp(temp->car->temp);
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
        // enterances
        failed += init_lpr(&data->enterances[i].lpr, &mutex_atr, &cond_atr);
        failed += init_boomgate(&data->enterances[i].boom, &mutex_atr, &cond_atr);
        failed += init_sign(&data->enterances[i].sign, &mutex_atr, &cond_atr);
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
    pthread_mutex_t mutex_pr;

} protected_rand;

// There were a few errors preventing compilation, commented out for now
/*
int random_parking_time(){
    //lock mutex
    pthread_mutex_lock(&pr->mutex_pr);
    int parking_time = rand() 10000+100;
    pthread_mutex_unlock(&pr->mutex_pr);
    
    return parking_time;

}

int random_car_creation_time(){
    //lock mutex
    pthread_mutex_lock(&pr->mutex_pr);
    int creation_time = rand() 100+1;
    pthread_mutex_unlock(&pr->mutex_pr);
    
    return creation_time;

}

int random_license_plate(protected_rand* pr){
    //lock mutex
    pthread_mutex_lock(&pr->mutex_pr);
    //Randomise if the car will have a valid license plate from hash table or a random plate
    //Random plate could be one of the list, but it is unlikely
    int valid = rand() % 2;
    

    char plate[];
    //char digits[10] = {"0123456789"};
    
    //if the car plate is random
    //License plates from the hash table are 3 numbers then 3
    //letters so we will asuume that format
    if valid == 0{
        //random plate

        for( i = 0 ; i < 3 ; i++ ) {
        strncat(plate, rand() % 10, 1);
        }

        for( i = 0 ; i < 3 ; i++ ) {
        strncat(plate,'A' + (rand() % 26, 1);
        }

    }

    //if from the hash table
    else {
        //plate from hash table
        plate = 0;
    }
    pthread_mutex_unlock(&pr->mutex_pr);
    return plate;
}*/

// add a car the the que for an entrance and trigger LPR if needed
// if cars already exist within the que, lpr is already triggered and the list will be cleared
void car_add(shared_data_t* data, car_t* car, int enterance) {
    car_entry_que[enterance] = l_list_add(car_entry_que[enterance], car);
    if (car_entry_que[enterance]->next == NULL) {
        usleep(2000);
        for (u_int8_t i = 0; i < 6; i++) {
            data->enterances[enterance].lpr.l_plate[i] = car->temp[i];
        }
        pthread_cond_broadcast(&data->enterances[enterance].lpr.cond);
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
    
    // Testing
    printf("Press ENTER to end the simulation\n");
    getchar();



    

    return 0;
}