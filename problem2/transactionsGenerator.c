#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define TRANSACTIONS_FILE "transactions.txt"
#define ACCOUNTS_FILE "accounts.txt"
#define NUM_TRANSACTIONS 100
#define AMOUNT_MIN 10.00
#define AMOUNT_MAX 1000.00

// Structure to hold account information
typedef struct {
    int account_number;
    double balance;
    double fee_percentage;
} Account;

// Function to generate a random double between min and max
double generate_random_double(double min, double max) {
    double scale = rand() / (double) RAND_MAX;
    return min + scale * (max - min);
}

// Function to read accounts from accounts.txt
int read_accounts(const char *filename, Account accounts[]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Unable to open %s\n", filename);
        return -1;
    }

    int count = 0;
    while (fscanf(file, "%d %lf %lf", &accounts[count].account_number, 
                  &accounts[count].balance, &accounts[count].fee_percentage) == 3) {
        count++;
        if (count >= 1000) { // Arbitrary limit to prevent overflow
            break;
        }
    }

    fclose(file);
    return count;
}

int main() {
    srand(time(NULL)); // Seed the random number generator

    Account accounts[1000];
    int account_count = read_accounts(ACCOUNTS_FILE, accounts);
    if (account_count <= 1) {
        printf("Error: Not enough accounts to generate transactions.\n");
        return 1;
    }

    FILE *file = fopen(TRANSACTIONS_FILE, "w");
    if (!file) {
        printf("Error: Unable to create %s\n", TRANSACTIONS_FILE);
        return 1;
    }

    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        // Select random source and destination ensuring they are different
        int src_idx = rand() % account_count;
        int dest_idx;
        do {
            dest_idx = rand() % account_count;
        } while (dest_idx == src_idx);

        int source = accounts[src_idx].account_number;
        int destination = accounts[dest_idx].account_number;

        double amount = generate_random_double(AMOUNT_MIN, AMOUNT_MAX);
        fprintf(file, "%06d %06d %.2lf\n", source, destination, amount);
    }

    fclose(file);
    printf("Successfully generated %d transactions in %s.\n", NUM_TRANSACTIONS, TRANSACTIONS_FILE);
    return 0;
}
