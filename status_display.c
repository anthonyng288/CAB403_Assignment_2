#include <stdio.h>
#include <sys/ioctl.h>  /* ioctl, TIOCGWINSZ */
#include <err.h>    /* err */
#include <fcntl.h>  /* open */
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "shm.h"
#include "defines.h"
int get_win_size()
{
    struct winsize ws;
    int fd;
    int cols;

    cols = 0;
    /* Open the controlling terminal. */
    fd = open("/dev/tty", O_RDWR);
    if (fd < 0)
        err(1, "/dev/tty");

    /* Get window size of terminal. */ 
    if (ioctl(fd, TIOCGWINSZ, &ws) < 0)
        err(1, "/dev/tty");
    close(fd);
    cols = ws.ws_col;
    return (cols);
}

void heading (char *heading, int col_width, int no_cols) {
    printf("\n");
    for(int i=0; i<no_cols; i++) {
        printf("%-*s", col_width-1, "------------------");
    }
    printf("\n");
    int spacing = 0;
    if(strcmp(heading, "Level") == 0) {
        spacing = col_width-7;
    }
    else if(strcmp(heading, "Entrance") == 0) {
        spacing = col_width-10;
    }
    else if(strcmp(heading, "Exit") == 0) {
        spacing = col_width-6;
    }
    for(int i=0; i<no_cols; i++) {
        printf("%s %-*d", heading, spacing, i+1);  
    }
    printf("\n");
    for(int i=0; i<MAX; i++) {
        printf("%-*s", col_width-1, "------------------");
    }
}
void Temp (int col_width, shared_mem_t* shm) {
    printf("\n");
    for(int i=0; i<LEVELS; i++) {
        printf("Temperature:    %02d%*s", shm->data->levels[i].temp, col_width-19, "");  
    }
}
void Capacity (int col_width, int levels_fullness[]) {
    printf("\n");
    for(int i=0; i<LEVELS; i++) {
        printf("Capacity:    %02d/%-*d", levels_fullness[i], col_width-17, LEVEL_CAPACITY);
    }
}
void LPR (char *heading, int col_width, shared_mem_t* shm) {
    printf("\n");
    if(strcmp(heading, "Entrance") == 0) {
        for(int i=0; i<ENTRANCES; i++) {
            printf("LPR:        %-*s", col_width-13, shm->data->enterances[i].lpr.l_plate);
        }
    }
    else if(strcmp(heading, "Level") == 0) {
        for(int i=0; i<LEVELS; i++) {
            printf("LPR:        %-*s", col_width-13, shm->data->levels[i].lpr.l_plate);
        }
    }
    else if(strcmp(heading, "Exit") == 0) {
        for(int i=0; i<EXITS; i++) {
            printf("LPR:        %-*s", col_width-13, shm->data->exits[i].lpr.l_plate);
        }
    } 
}
void Boomgate (char *heading, int col_width, shared_mem_t* shm) {
    printf("\n");
    if(strcmp(heading, "Entrance") == 0) {
        for(int i=0; i<ENTRANCES; i++) {
            printf("Boom gate:       %-*s", col_width-18, &shm->data->enterances[i].boom.status);
        }
    }
    else if(strcmp(heading, "Exit") == 0) {
        for(int i=0; i<EXITS; i++) {
            printf("Boom gate:       %-*s", col_width-18, &shm->data->exits[i].boom.status);
        }
    } 
}
void Screen (int col_width, shared_mem_t* shm) {
    printf("\n");
    for(int i=0; i<ENTRANCES; i++) {
        printf("Screen:          %-*s", col_width-18, &shm->data->enterances[i].sign.display);
    }
}
void print_levels(int levels_fullness[], int col_width, shared_mem_t* shm) {
    heading("Level", col_width, LEVELS);
    Temp(col_width, shm); 
    Capacity(col_width, levels_fullness);
    LPR("Level", col_width, shm);
    printf("\r\n");
}

void print_entrances(int col_width, shared_mem_t* shm) {
    heading("Entrance", col_width, ENTRANCES);
    LPR("Entrance", col_width, shm);
    Boomgate("Entrance",col_width, shm);
    Screen(col_width, shm);
    printf("\r\n");
}

void print_exits(int col_width, shared_mem_t* shm) {
    heading("Exit", col_width, EXITS);
    LPR("Exit", col_width, shm);
    Boomgate("Exit", col_width, shm);
    printf("\r\n");
}
void print_revenue() {
    printf(" \n");
    printf(" ---------------- \n");
    printf("| Revenue:  $%.2d|\n", 2000);
    printf(" ---------------- ");
}
void status_display(int levels_fullness[], shared_mem_t* shm){
    int screen_size = get_win_size(); 
    int col_width = screen_size/MAX;  
    
    print_levels(levels_fullness, col_width, shm);
    print_entrances(col_width, shm);
    print_exits(col_width, shm);

    print_revenue();
}