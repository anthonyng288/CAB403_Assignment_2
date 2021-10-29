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