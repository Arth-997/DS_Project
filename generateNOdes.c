#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_NODES 30

// Function to generate accounts with 6-digit IDs and save them to a file
void generate_accounts_file(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error opening file %s\n", filename);
        return;
    }

    for (int i = 0; i < MAX_NODES; i++) {
        char account_id[7];
        sprintf(account_id, "%06d", i + 1);         // Account ID from 000001 to 000030
        double balance = (rand() % 4500) + 500;     // Random balance between 500 and 5000
        double fee_percentage = (rand() % 46) / 10.0 + 0.5;  // Random fee percentage between 0.5% and 5.0%
        fprintf(file, "%s %.2f %.2f\n", account_id, balance, fee_percentage);
    }

    fclose(file);
    printf("Generated %d accounts in %s\n", MAX_NODES, filename);
}
#define NUM_TRANSACTIONS 50

// Function to generate a random 6-digit transaction ID
void generate_random_txn_id(char* txn_id) {
    sprintf(txn_id, "%06d", rand() % 900000 + 100000);
}

// Function to generate random transactions and save them to a file
void generate_transactions_file(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error opening file %s\n", filename);
        return;
    }

    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        char txn_id[7];
        generate_random_txn_id(txn_id);

        int src = rand() % MAX_NODES + 1;      // Random source node ID (000001 to 000030)
        int dest;
        do {
            dest = rand() % MAX_NODES + 1;     // Random destination node ID (different from src)
        } while (dest == src);
        
        double amount = (rand() % 900) + 100;  // Random amount between 100 and 1000

        fprintf(file, "%s %06d %06d %.2f\n", txn_id, src, dest, amount);
    }

    fclose(file);
    printf("Generated %d transactions in %s\n", NUM_TRANSACTIONS, filename);
}
int main() {
    srand(time(NULL));

    // Generate accounts and transactions
    generate_accounts_file("accounts.txt");
    generate_transactions_file("transactions.txt");

    return 0;
}
