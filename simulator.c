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

typedef struct car {
    char* lp;
    time_t enter_time;
} car_t;


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
node_t* car_entry_queue[ENTRANCES];

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
        car_entry_queue[i] = NULL; // car que
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
////       Random           ////
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
    
    
    if (valid == 0){car_entry
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

// //Tell when to open boomgates
// pc_boom_t boomgates;
// void boomgate_func_open(pc_boom_t boomgate_protocol){
//         pthread_mutex_lock(&boomgate_protocol.lock);
//         if(boomgate_protocol.status == 'R'){
//             // change the status to "O" after 10 milli
//             sleeping_beauty(10);
//             boomgate_protocol.status = 'O';
//             //Make these functions WAIT for signal from manager
//             pthread_cond_wait(boomgate_protocol.cond, boomgate_protocol.lock);
//         }
//         pthread_mutex_unlock(&boomgate_protocol.lock);

          
// }

// //Tell when to close boosmgates
// void boomgate_func_close(pc_boom_t boomgate_protocol){
//     pthread_mutex_lock(&boomgate_protocol.lock);

//         if(boomgate_protocol.status == 'L'){
//             // change the status to "O" after 10 milli
//             sleeping_beauty(10);
//             boomgate_protocol.status = 'C';
//         }
//         pthread_mutex_unlock(&boomgate_protocol.lock);

// }

// void boomgate_process(pc_boom_t boomgates){
//      //Probably very bad practice, will need to see
//     boomgate_func_open(boomgates);
//     usleep(20);
//     boomgate_func_close(boomgates);
// }

// add a car the the que for an entrance and trigger LPR if needed
// if cars already exist within the que, lpr is already triggered and the list will be cleared
void car_add(shared_data_t* data, char licence[6], int entry) {
    car_entry_queue[entry] = l_list_add(car_entry_queue[entry], licence);
    if (car_entry_queue[entry]->next == NULL)
    {
        for (int8_t i = 5; i >= 0; i--) {
            data->entrances[entry].lpr.l_plate[i] = licence[i];
        }
        pthread_cond_broadcast(&data->entrances[entry].lpr.cond);
    }
}

typedef struct car_enty_struct car_entry_struct_t;
struct car_enty_struct {
    shared_data_t* data;
    int entry;
};
void boom_handler(p_enterance_t* ent){
    while (1)
    {
        // wait until a car is waiting to begin the open cycle
        while (ent->boom.status != 'R') {
            pthread_cond_wait(&ent->boom.cond, & ent->boom.lock);
        }
        usleep(10000); // wait for gate to open
        ent->boom.status = 'O';
        printf("SET OPEN\n");
        pthread_cond_broadcast(&ent->boom.cond);
        while (ent->boom.status != 'L') {
            pthread_cond_wait(&ent->boom.cond, & ent->boom.lock);
        }
        usleep(10000); // wait for gate to close
        ent->boom.status = 'C';
        printf("SET CLOSE\n");
        ent->sign.display = '\0';
        pthread_cond_broadcast(&ent->boom.cond);
    }
}

void car_entry(car_entry_struct_t* input) {
    p_enterance_t* ent = &input->data->entrances[input->entry];
    pthread_mutex_lock(&ent->sign.lock);
    while (1)
    {
        while (ent->sign.display == '\0') {
            
            pthread_cond_wait(&ent->sign.cond, &ent->sign.lock);
        }
        if (ent->sign.display == 'X') {
            // Turn car away, car is deleted
            car_entry_queue[input->entry] = l_list_remove(car_entry_queue[input->entry]);
        } else {
            while (ent->boom.status == 'L') {
                usleep(1000);
            }
            
            // If a car is accepted this code will be reaced.
            // Impletement way to determine state of gate before car is accepted
            if (ent->boom.status == 'C' && ent->sign.display != '\0')
            {
                pthread_cond_broadcast(&ent->boom.cond);
                // wait 1 ms to allow the threads to detect they are signaled before
                // the display is cleared (doesn't effect timings)
                // We might want to try to make this better later
                usleep(1000);
            }


            while (ent->boom.status != 'O' && ent->sign.display != '\0') {
                usleep(1000);
            }
            
            // create car struct
            if (ent->sign.display != '\0') {
                car_entry_queue[input->entry] = l_list_remove(car_entry_queue[input->entry]);
                printf("CAR IS IN\n");
            }
        }
        ent->sign.display = '\0';
        if (car_entry_queue[input->entry]!= NULL) {
            for (int8_t i = 5; i >= 0; i--) {
                ent->lpr.l_plate[i] = car_entry_queue[input->entry]->licence[i];
            }
            usleep(2000);
            pthread_cond_broadcast(&ent->lpr.cond);
        }
    }
    pthread_mutex_unlock(&ent->sign.lock);
}
typedef struct thread_var {
    car_entry_struct_t entrance_vars[ENTRANCES];
} thread_var_t;

typedef struct thread_list {
    pthread_t entrance_threads[ENTRANCES];
} thread_list_t;

int init_threads(thread_list_t* t_list, thread_var_t* t_var, shared_data_t* data){
    for (int i = 0; i < ENTRANCES; i++) {
        t_var->entrance_vars[i].data = data;
        t_var->entrance_vars[i].entry = i;
        if (pthread_create(&t_list->entrance_threads[i], NULL, (void*)car_entry,
        &t_var->entrance_vars[i])){
            return EXIT_FAILURE;
        }
        if (pthread_create(&t_list->entrance_threads[i], NULL, (void*)boom_handler,
        &data->entrances[i])){
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

// void car_generator(protect_rand_t pr){

//     int car_creation_time = random_car_creation_time(pr);
//     usleep(car_creation_time);

//     car_t car; //Name needs to be dynamic
//     char* plate = NULL;
//     plate = "ABC123"; //Needs to be replaced with actual plate function

//     time_t parking_time = random_parking_time(pr); //May need to change time_t to int may not need time_t

    
//     // car_add(shm.data, plate, parking_time);
//     // free(plate);

    
// }

int main()
{
    //protect_rand_t pr;
    shared_mem_t sh_mem; // shared memory
    thread_var_t thread_vars;
    thread_list_t thread_lists;

    protect_rand_t pr;
    pthread_mutex_init(&pr.lock, NULL);
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
    
    init_threads(&thread_lists, &thread_vars, sh_mem.data);

    getchar();
    char tempzz[6] = {'3', '7', '6', 'D', 'D', 'S'};
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    car_add(sh_mem.data, tempzz, 0);
    getchar();

    return EXIT_SUCCESS;
}