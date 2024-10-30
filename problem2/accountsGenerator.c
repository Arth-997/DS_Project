#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define NUM_ACCOUNTS 50
#define ACCOUNT_NUM_MIN 100000
#define ACCOUNT_NUM_MAX 999999
#define BALANCE_MIN 1000.00
#define BALANCE_MAX 10000.00
#define FEE_MIN 1.0
#define FEE_MAX 5.0
#define ACCOUNTS_FILE "accounts.txt"

// Function to generate a random double between min and max
double generate_random_double(double min, double max) {
    double scale = rand() / (double) RAND_MAX;
    return min + scale * (max - min);
}

// Function to check if an account number already exists
bool account_exists(int account_numbers[], int count, int number) {
    for (int i = 0; i < count; i++) {
        if (account_numbers[i] == number)
            return true;
    }
    return false;
}

int main() {
    FILE *file = fopen(ACCOUNTS_FILE, "w");
    if (!file) {
        printf("Error: Unable to create %s\n", ACCOUNTS_FILE);
        return 1;
    }

    srand(time(NULL)); // Seed the random number generator

    int account_numbers[NUM_ACCOUNTS];
    int count = 0;

    while (count < NUM_ACCOUNTS) {
        int acc_num = ACCOUNT_NUM_MIN + rand() % (ACCOUNT_NUM_MAX - ACCOUNT_NUM_MIN + 1);
        if (!account_exists(account_numbers, count, acc_num)) {
            account_numbers[count] = acc_num;
            double balance = generate_random_double(BALANCE_MIN, BALANCE_MAX);
            double fee = generate_random_double(FEE_MIN, FEE_MAX);
            fprintf(file, "%06d %.2lf %.2lf\n", acc_num, balance, fee);
            count++;
        }
    }

    fclose(file);
    printf("Successfully generated %d accounts in %s.\n", NUM_ACCOUNTS, ACCOUNTS_FILE);
    return 0;
}
