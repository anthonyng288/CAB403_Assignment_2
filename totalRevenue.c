// Calculates the total revenue for the car park
double calculate_total_revenue() {
    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    double revenue = 0.0;
    
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
        return revenue;  
    }
    else {
        printf("File does not exist.");
        return 0;
    }    
}
