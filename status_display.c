#include <stdio.h>
#include <sys/ioctl.h>  /* ioctl, TIOCGWINSZ */
#include <err.h>    /* err */
#include <fcntl.h>  /* open */
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "shm.h"

#define MAX 5
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
    for(int i=0; i<no_cols; i++) {
        printf("%-*s", col_width-1, "------------------");
    }
}
void Temp (int col_width, int no_cols, int16_t level_temp) {
    printf("\n");
    for(int i=0; i<no_cols; i++) {
        printf("Temperature:    %02d%*s", level_temp, col_width-19, "");  
    }
}
void Capacity (int col_width, int no_cols, int level_capacity, int levels_fullness[]) {
    printf("\n");
    for(int i=0; i<no_cols; i++) {
        printf("Capacity:    %02d/%-*d", levels_fullness[i], col_width-17, level_capacity);
    }
}
void LPR (int col_width, int no_cols, pc_lpr_t* lpr) {
    printf("\n");
    for(int i=0; i<no_cols; i++) {
        printf("LPR:             %-*s", col_width-18, lpr->l_plate);
    }
}
void Boomgate (int col_width, int no_cols, pc_boom_t* boom) {
    printf("\n");
    for(int i=0; i<no_cols; i++) {
        printf("Boom gate:       %-*s", col_width-18, &boom->status);
    }
}
void Screen (int col_width, int no_cols, pc_sign_t* screen) {
    printf("\n");
    for(int i=0; i<no_cols; i++) {
        printf("Screen:          %-*s", col_width-18, &screen->display);
    }
}
void print_levels(int num_levels, int levels_fullness[], int level_capacity, int col_width, p_level_t* level_info) {
    heading("Level", col_width, num_levels);
    Temp(col_width, num_levels, level_info->temp); 
    Capacity(col_width, num_levels, level_capacity, levels_fullness);
    LPR(col_width, num_levels, &level_info->lpr);
    printf("\r\n");
}

void print_entrances(int num_entrances, int col_width, p_enterance_t* entrance_info) {
    heading("Entrance", col_width, num_entrances);
    LPR(col_width, num_entrances, &entrance_info->lpr);
    Boomgate(col_width, num_entrances, &entrance_info->boom);
    Screen(col_width, num_entrances, &entrance_info->sign);
    printf("\r\n");
}

void print_exits(int num_exits, int col_width, p_exit_t* exit_info) {
    heading("Exit", col_width, num_exits);
    LPR(col_width, num_exits, &exit_info->lpr);
    Boomgate(col_width, num_exits, &exit_info->boom);
    printf("\r\n");
}
void print_revenue(double revenue) {
    printf(" \n");

    printf(" ---------------- \n");
    printf("| Revenue:  $%.2f|\n", revenue);
    printf(" ---------------- ");
}
void status_display(int num_levels, int levels_fullness[], int num_entrances, int num_exits, int level_capacity, double revenue, shared_mem_t* shm){
    int screen_size = get_win_size(); 
    int col_width = screen_size/MAX;  
    // printf("\e[1;1H\e[2J");
    print_levels(num_levels, levels_fullness, level_capacity, col_width, shm->data->levels);
    print_entrances(num_entrances, col_width, shm->data->enterances);
    print_exits(num_exits, col_width, shm->data->exits);

    print_revenue(revenue);
}