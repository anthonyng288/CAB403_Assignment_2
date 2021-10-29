// Change temperature and simulate fire (incomplete)
void rand_temp(shared_mem_t *shm) {
    srand(time(NULL));
    // for(int i = 0; i<LEVELS; i++) {
    //     shm->data->levels[i].temp = 20;
    // }
    if(shm->data->levels[0].temp == 0) {
        int temp = (rand() % (35 + 1 - 10)) + 10;
        for(int i = 0; i<LEVELS; i++) {
            shm->data->levels[i].temp = temp;
            printf("Temp for level %d is: %d", i+1, shm->data->levels[i].temp);
        }
    }
    else {
        int inc_dec = (rand() % (1 + 1 - -1)) + -1;
        for(int i = 0; i<LEVELS; i++) {
            int change = rand() % 3;
            if(shm->data->levels[i].temp > 42 && inc_dec != 1) {
                shm->data->levels[i].temp = shm->data->levels[i].temp + (inc_dec*change);
            }
            if(shm->data->levels[i].temp < 10 && inc_dec != -1){
                shm->data->levels[i].temp = shm->data->levels[i].temp + (inc_dec*change);
            }
            else if(shm->data->levels[i].temp < 45 && shm->data->levels[i].temp > 7) {
                shm->data->levels[i].temp = shm->data->levels[i].temp + (inc_dec*change);
            }
        }
    }
}