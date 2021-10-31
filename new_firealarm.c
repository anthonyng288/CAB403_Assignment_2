#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "shm.h"
#include "defines.h"


int shm_fd;
shared_data_t *shm_data;

int compare(const void *first, const void *second)
{
	return *((const int *)first) - *((const int *)second);
}

// Calculate the median of an array of 5 signed integers
// perams:
//      input: an array of 5 int16_t must be passed
int16_t calc_median(int16_t input[5]){
    qsort(input, 5, sizeof(int16_t), compare);
    return input[3]; // return middle value
}

// Check to see if there conditions for a fire are met in any level
// perams:
//      data: an array with the smoothed data for the carpark
//      levels: the number of levels in the carpark
//      pos: the positions in the data array with the newest entry  
bool check_fire(int16_t data[][30], u_int8_t pos){
    u_int8_t oldest_pos; // position of oldest data value
    if (pos == 0) {
        oldest_pos = 29;
    } else {
        oldest_pos = pos - 1;
    }
    
    for (u_int8_t i = 0; i < LEVELS; i++) {
        printf("Difference in temp is: %d", data[i][pos] - data[i][oldest_pos]);

        if (data[i][pos] - data[i][oldest_pos] >= 8){
            printf("Difference in temp is: %d", data[i][pos] - data[i][oldest_pos]);
            return true;
        }
        
    }
    return false;
}

// Monitor the carpark and exit when a fire is found
// perams:
//      levels: the number of levels in the carpark
void tempmonitor(bool* fire){
    // store raw data
    printf("\n(tempmonitor - before)Fire bool status: %s", fire ? "True" : "False");

    *fire = false;
    printf("\n(tempmonitor - after)Fire bool status: %s", fire ? "True" : "False");

    int16_t temp_data[LEVELS][5];
    u_int8_t data_pos = 0;
    // store smoothed data
    int16_t smooth_data[LEVELS][30];
    u_int8_t smooth_data_pos = 0;
    printf("\n(tempmonitor)Fire bool status: %s", fire ? "True" : "False");

    struct timespec sleep_rem = {0, 2000000}; // 20 ms
    struct timespec sleep_req = {0, 2000000};

    // read 4 variables before a smoothed value can be calculated
    for (u_int8_t i = 0; i < 4; i++) {
        // Store raw data from every level
        for (u_int8_t j = 0; j < LEVELS; j++) {
            temp_data[j][data_pos] = shm_data->levels[j].temp;
            // printf("\n(4V)(Level %d)Temp: %d", j+1, temp_data[j][data_pos]);

        }
        data_pos++; // will not go out of bounds in loop
        nanosleep(&sleep_req, &sleep_rem);
    }
    // read 30 variables and calculate smoothed variables
    for (u_int8_t i = 0; i < 30; i++) {
        for (u_int8_t j = 0; j < LEVELS; j++) {
            temp_data[j][data_pos] = shm_data->levels[j].temp;
            smooth_data[j][smooth_data_pos] = calc_median(temp_data[j]);
            // printf("\n(30V)(Level %d)Temp: %d", j+1, smooth_data[j][smooth_data_pos]);

        }
        // Modulus is avoided to prevent integer overflow error
        if (data_pos >= 4) {
            data_pos = 0;
        } else {
            data_pos++;
        }
        smooth_data_pos++; // will not go out of bounds in loop
        nanosleep(&sleep_req, &sleep_rem);
    }
    // read variables and determine if there should be a fire
    while (!fire)
    {
        printf("\nYou are in while(!fire)");
        for (u_int8_t i = 0; i < LEVELS; i++) {
            temp_data[i][data_pos] = shm_data->levels[i].temp;
            smooth_data[i][smooth_data_pos] = calc_median(temp_data[i]);
        }
        if (data_pos >= 4) {
            data_pos = 0;
        } else {
            data_pos++;
        }
        if (smooth_data_pos >= 29){
            smooth_data_pos = 0;
        }
        *fire = check_fire(smooth_data, smooth_data_pos);  
    }
    return;   
}

void update_signs(shared_data_t* parking, char input){
    // printf("You are in update_signs");

    for (u_int8_t i = 0; i < ENTRANCES; i++) {
        pthread_mutex_lock(&parking->entrances[i].sign.lock);
        parking->entrances[i].sign.display = input;
        pthread_mutex_unlock(&parking->entrances[i].sign.lock);
    }
    return;
}

void open_gates(){
    // printf("You are in open_gates()");
    // Todo
    return;
}


void evacuate_garage(shared_data_t* parking, bool* fire){
    // printf("You are in evacuate_garage()");
    struct timespec wait_req, wait_rem = {0, 20000000}; // 20 ms
    char evacmessage[9] = {'E', 'V', 'A', 'C', 'U', 'A', 'T', 'E', ' '};
    u_int8_t evac_pos = 0;

    while (fire) {
        open_gates();
        // printf("THERE IS A FIRE");
        update_signs(parking, (evacmessage[evac_pos]));
        // prevent integer overflow error
        if (evac_pos >= 8) {
            evac_pos = 0;
        } else {
            evac_pos++;
        }
        nanosleep(&wait_req, &wait_rem);
    }
    return;
}

int main(void){
    bool fire = false;
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
	shm_data = (void *) mmap(0, sizeof(shm_fd), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    printf("\n(Before)Fire bool status: %s", fire ? "True" : "False");

    printf("\n(After)Fire bool status: %s", fire ? "True" : "False");

    // wait until a fire is detected
    tempmonitor(&fire);
    printf("\n(End)Fire bool status: %s", fire ? "True" : "False");


    // run until fire is gone
    // evacuate_garage(shm_data, &fire);

    munmap((void *)shm_data, 2920);
	close(shm_fd);
    return EXIT_SUCCESS;
}