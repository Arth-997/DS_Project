#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define MAX_ACCOUNTS 1000
#define MAX_TRANSACTIONS 10000
#define ACCOUNT_NUM_LENGTH 6
#define STATE_FILE "state.dat"
#define ACCOUNTS_FILE "accounts.txt"
#define TRANSACTIONS_FILE "transaction.txt"

#define INF 1e9

// Structure to hold account information
typedef struct {
    int account_number; // 6-digit
    double balance;
    double fee_percentage;
} Account;

// Structure to hold transaction information
typedef struct {
    int transaction_id;
    int source;
    int destination;
    double amount;
    double fee;
    char path[256]; // Simple representation of path
} Transaction;

// Global variables
Account accounts[MAX_ACCOUNTS];
int account_count = 0;

// Adjacency matrix for fees (-1 indicates no connection)
double graph[MAX_ACCOUNTS][MAX_ACCOUNTS] = { { -1 } };

// Transaction history loaded from state.dat
Transaction transaction_history[MAX_TRANSACTIONS];
int history_count = 0;

// New transactions loaded from transactions.txt
Transaction new_transactions[MAX_TRANSACTIONS];
int new_transaction_count = 0;

// Function to find account index by account number
int find_account_index(int account_number) {
    for (int i = 0; i < account_count; i++) {
        if (accounts[i].account_number == account_number)
            return i;
    }
    return -1;
}

// Function to add a new account
int add_account(int account_number, double balance, double fee_percentage) {
    if (account_count >= MAX_ACCOUNTS) {
        printf("Error: Maximum number of accounts (%d) reached.\n", MAX_ACCOUNTS);
        return -1;
    }
    accounts[account_count].account_number = account_number;
    accounts[account_count].balance = balance;
    accounts[account_count].fee_percentage = fee_percentage;
    
    // Initialize graph connections for the new account
    for (int i = 0; i <= account_count; i++) {
        if (i < account_count) {
            graph[account_count][i] = -1; // No connection initially
            graph[i][account_count] = -1;
        }
    }
    graph[account_count][account_count] = 0; // Self-loop with zero fee
    account_count++;
    return account_count - 1;
}

// Function to load accounts from accounts.txt
bool load_accounts_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Failed to open %s\n", filename);
        return false;
    }
    int acc_num;
    double balance, fee;
    while (fscanf(file, "%d %lf %lf", &acc_num, &balance, &fee) == 3) {
        if (add_account(acc_num, balance, fee) == -1) {
            fclose(file);
            return false;
        }
    }
    fclose(file);
    printf("Loaded %d accounts from %s.\n", account_count, filename);
    return true;
}

// Function to save state to state.dat
bool save_state(const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Error: Failed to open %s for writing.\n", filename);
        return false;
    }
    // Write account count
    fwrite(&account_count, sizeof(int), 1, file);
    // Write accounts
    fwrite(accounts, sizeof(Account), account_count, file);
    // Write graph
    fwrite(graph, sizeof(double), MAX_ACCOUNTS * MAX_ACCOUNTS, file);
    // Write transaction history count
    fwrite(&history_count, sizeof(int), 1, file);
    // Write transaction history
    fwrite(transaction_history, sizeof(Transaction), history_count, file);
    fclose(file);
    printf("State saved to %s successfully.\n", filename);
    return true;
}

// Function to load state from state.dat
bool load_state(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("State file %s does not exist. Proceeding without it.\n", filename);
        return false;
    }
    // Read account count
    if (fread(&account_count, sizeof(int), 1, file) != 1) {
        printf("Error: Failed to read account count from %s.\n", filename);
        fclose(file);
        return false;
    }
    if (account_count > MAX_ACCOUNTS) {
        printf("Error: Account count in state file exceeds maximum allowed.\n");
        fclose(file);
        return false;
    }
    // Read accounts
    if (fread(accounts, sizeof(Account), account_count, file) != (size_t)account_count) {
        printf("Error: Failed to read accounts from %s.\n", filename);
        fclose(file);
        return false;
    }
    // Read graph
    if (fread(graph, sizeof(double), MAX_ACCOUNTS * MAX_ACCOUNTS, file) != (size_t)(MAX_ACCOUNTS * MAX_ACCOUNTS)) {
        printf("Error: Failed to read graph from %s.\n", filename);
        fclose(file);
        return false;
    }
    // Read transaction history count
    if (fread(&history_count, sizeof(int), 1, file) != 1) {
        printf("Error: Failed to read transaction history count from %s.\n", filename);
        fclose(file);
        return false;
    }
    if (history_count > MAX_TRANSACTIONS) {
        printf("Error: Transaction history count in state file exceeds maximum allowed.\n");
        fclose(file);
        return false;
    }
    // Read transaction history
    if (fread(transaction_history, sizeof(Transaction), history_count, file) != (size_t)history_count) {
        printf("Error: Failed to read transaction history from %s.\n", filename);
        fclose(file);
        return false;
    }
    fclose(file);
    printf("State loaded from %s successfully.\n", filename);
    return true;
}

// Function to load program state
bool load_program_state() {
    return load_state(STATE_FILE);
}

// Function to save program state
bool save_program_state() {
    return save_state(STATE_FILE);
}

// Function to add edge to graph
void add_edge(int src_idx, int dest_idx) {
    if (src_idx < 0 || src_idx >= account_count || dest_idx < 0 || dest_idx >= account_count) {
        printf("Error: Invalid indices while adding edge.\n");
        return;
    }
    // Since edges are undirected, set both [src][dest] and [dest][src]
    graph[src_idx][dest_idx] = accounts[dest_idx].fee_percentage;
    graph[dest_idx][src_idx] = accounts[src_idx].fee_percentage;
    printf("Edge added between %06d and %06d with fees %.2lf%% and %.2lf%% respectively.\n",
           accounts[src_idx].account_number, accounts[dest_idx].account_number,
           accounts[dest_idx].fee_percentage, accounts[src_idx].fee_percentage);
}

// Function to read new transactions from transactions.txt
bool load_new_transactions(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Warning: Failed to open %s. No new transactions to process.\n", filename);
        return false;
    }
    int src, dest;
    double amount;
    while (fscanf(file, "%d %d %lf", &src, &dest, &amount) == 3) {
        if (new_transaction_count >= MAX_TRANSACTIONS) {
            printf("Warning: Maximum number of new transactions (%d) reached. Skipping remaining.\n", MAX_TRANSACTIONS);
            break;
        }
        // Validate account numbers
        int src_idx = find_account_index(src);
        int dest_idx = find_account_index(dest);
        if (src_idx == -1 || dest_idx == -1) {
            printf("Warning: Transaction with invalid account numbers %06d -> %06d skipped.\n", src, dest);
            continue;
        }
        // Prevent transactions to the same account
        if (src == dest) {
            printf("Warning: Transaction with identical source and destination %06d skipped.\n", src);
            continue;
        }
        new_transactions[new_transaction_count].transaction_id = history_count + new_transaction_count + 1;
        new_transactions[new_transaction_count].source = src;
        new_transactions[new_transaction_count].destination = dest;
        new_transactions[new_transaction_count].amount = amount;
        new_transactions[new_transaction_count].fee = 0.0;
        strcpy(new_transactions[new_transaction_count].path, "");
        new_transaction_count++;
    }
    fclose(file);
    if (new_transaction_count > 0)
        printf("Loaded %d new transactions from %s.\n", new_transaction_count, filename);
    else
        printf("No valid new transactions found in %s.\n", filename);
    return true;
}

// Function to perform Dijkstra's algorithm to find the path with minimum total fee
bool dijkstra(int src_idx, int dest_idx, int *path, double *total_fee) {
    // Initialize distance and predecessor arrays
    double dist[MAX_ACCOUNTS];
    int pred[MAX_ACCOUNTS];
    bool visited[MAX_ACCOUNTS];
    
    for (int i = 0; i < account_count; i++) {
        dist[i] = INF;
        pred[i] = -1;
        visited[i] = false;
    }
    
    dist[src_idx] = 0.0;
    
    for (int i = 0; i < account_count; i++) {
        // Find the unvisited node with the smallest distance
        double min_dist = INF;
        int u = -1;
        for (int j = 0; j < account_count; j++) {
            if (!visited[j] && dist[j] < min_dist) {
                min_dist = dist[j];
                u = j;
            }
        }
        
        if (u == -1) break; // All reachable nodes visited
        
        visited[u] = true;
        
        // Update distances to neighbors
        for (int v = 0; v < account_count; v++) {
            if (graph[u][v] >= 0 && !visited[v]) {
                double new_dist = dist[u] + graph[u][v];
                if (new_dist < dist[v]) {
                    dist[v] = new_dist;
                    pred[v] = u;
                }
            }
        }
    }
    
    if (dist[dest_idx] == INF) {
        // No path found
        return false;
    }
    
    // Reconstruct the path
    int count = 0;
    int u = dest_idx;
    while (u != src_idx) {
        if (u == -1) break; // Should not happen
        path[count++] = u;
        u = pred[u];
    }
    path[count++] = src_idx;
    
    // Reverse the path to get from src to dest
    for (int i = 0; i < count / 2; i++) {
        int temp = path[i];
        path[i] = path[count - i - 1];
        path[count - i - 1] = temp;
    }
    
    // Calculate total fee
    double fee_sum = 0.0;
    for (int i = 0; i < count - 1; i++) {
        fee_sum += graph[path[i]][path[i+1]];
    }
    *total_fee = fee_sum;
    
    return true;
}

// Function to process a single transaction
void process_transaction(Transaction *txn) {
    int src_idx = find_account_index(txn->source);
    int dest_idx = find_account_index(txn->destination);
    if (src_idx == -1 || dest_idx == -1) {
        printf("Error: Invalid account number in transaction ID %d.\n", txn->transaction_id);
        return;
    }
    
    // Find path with minimum total fee using Dijkstra's algorithm
    int path[MAX_ACCOUNTS];
    double total_fee;
    bool path_found = dijkstra(src_idx, dest_idx, path, &total_fee);
    
    if (!path_found) {
        // No path exists, add a direct edge
        add_edge(src_idx, dest_idx);
        // After adding the edge, attempt to find the path again
        path_found = dijkstra(src_idx, dest_idx, path, &total_fee);
        if (!path_found) {
            printf("Error: Unable to find path even after adding edge for transaction ID %d.\n", txn->transaction_id);
            return;
        }
    }
    
    // Calculate fee
    double fee = txn->amount * (total_fee / 100.0);
    
    // Check if source has sufficient balance
    if (accounts[src_idx].balance < txn->amount) {
        printf("Error: Insufficient balance in account %06d for transaction ID %d.\n",
               accounts[src_idx].account_number, txn->transaction_id);
        return;
    }
    
    // Update balances
    accounts[src_idx].balance -= txn->amount;
    accounts[dest_idx].balance += (txn->amount - fee);
    
    // Distribute fees to intermediary accounts
    // Exclude source and destination
    for (int i = 1; i < account_count && path[i] != dest_idx; i++) {
        int intermediary_idx = path[i];
        accounts[intermediary_idx].balance += (txn->amount * (graph[path[i-1]][path[i]] / 100.0));
    }
    
    // Store fee and path in transaction
    txn->fee = fee;
    
    // Build path string
    char path_str[256] = "";
    for (int i = 0; i < account_count && path[i] != dest_idx && path[i] != -1; i++) {
        char num_str[7];
        sprintf(num_str, "%06d", accounts[path[i]].account_number);
        strcat(path_str, num_str);
        if (path[i + 1] != dest_idx && path[i + 1] != -1)
            strcat(path_str, "->");
        else if (path[i + 1] == dest_idx)
            sprintf(path_str + strlen(path_str), "->%06d", accounts[path[i + 1]].account_number);
        else
            break;
    }
    strcpy(txn->path, path_str);
    
    printf("Processed Transaction ID %d: %06d -> %06d | Amount: %.2lf | Fee: %.2lf | Path: %s\n",
           txn->transaction_id, txn->source, txn->destination, txn->amount, txn->fee, txn->path);
    
    // Append to transaction history
    if (history_count >= MAX_TRANSACTIONS) {
        printf("Warning: Maximum transaction history reached. Cannot store new transactions.\n");
        return;
    }
    transaction_history[history_count++] = *txn;
}

// Function to process all new transactions
void process_all_new_transactions() {
    for (int i = 0; i < new_transaction_count; i++) {
        process_transaction(&new_transactions[i]);
    }
}

// Function to display transaction details
void display_transaction(Transaction *txn) {
    printf("Transaction ID: %d\n", txn->transaction_id);
    printf("Source: %06d\n", txn->source);
    printf("Destination: %06d\n", txn->destination);
    printf("Amount: %.2lf\n", txn->amount);
    printf("Fee: %.2lf\n", txn->fee);
    printf("Path: %s\n", txn->path);
    printf("------------------------\n");
}

// Function to fetch transactions by account number
void fetch_transactions_by_account(int account_number) {
    printf("Transactions for account %06d:\n", account_number);
    bool found = false;
    for (int i = 0; i < history_count; i++) {
        if (transaction_history[i].source == account_number || transaction_history[i].destination == account_number) {
            display_transaction(&transaction_history[i]);
            found = true;
        }
    }
    if (!found) {
        printf("No transactions found for account %06d.\n", account_number);
    }
}

// Function to fetch transaction by transaction ID
void fetch_transaction_by_id(int txn_id) {
    for (int i = 0; i < history_count; i++) {
        if (transaction_history[i].transaction_id == txn_id) {
            display_transaction(&transaction_history[i]);
            return;
        }
    }
    printf("Transaction ID %d not found.\n", txn_id);
}

// Function to print all accounts
void print_all_accounts() {
    printf("All Accounts:\n");
    printf("Account Number | Balance     | Fee Percentage\n");
    printf("--------------------------------------------\n");
    for (int i = 0; i < account_count; i++) {
        printf("%06d         | %.2lf      | %.2lf%%\n",
               accounts[i].account_number,
               accounts[i].balance,
               accounts[i].fee_percentage);
    }
    printf("------------------------\n");
}

// Main Function
int main() {
    // Load state if exists, else load from accounts.txt
    FILE *state_check = fopen(STATE_FILE, "rb");
    if (state_check) {
        fclose(state_check);
        if (load_program_state()) {
            printf("Loaded state from %s.\n", STATE_FILE);
        } else {
            printf("Failed to load state from %s. Exiting.\n", STATE_FILE);
            return 1;
        }
    } else {
        if (!load_accounts_from_file(ACCOUNTS_FILE)) {
            printf("Failed to load accounts from %s. Exiting.\n", ACCOUNTS_FILE);
            return 1;
        }
    }

    // Load new transactions
    if (load_new_transactions(TRANSACTIONS_FILE)) {
        printf("Loaded %d new transactions from %s.\n", new_transaction_count, TRANSACTIONS_FILE);
    }

    // Process transactions if any
    if (new_transaction_count > 0) {
        process_all_new_transactions();
        printf("Processed all new transactions.\n");
    } else {
        printf("No new transactions to process.\n");
    }

    // Save state
    if (save_program_state()) {
        printf("Saved state to %s.\n", STATE_FILE);
    } else {
        printf("Failed to save state to %s.\n", STATE_FILE);
    }

    // Simple Menu for fetching transactions
    int choice;
    while (1) {
        printf("\nMenu:\n");
        printf("1. Display all accounts\n");
        printf("2. Fetch transactions by account number\n");
        printf("3. Fetch transaction by transaction ID\n");
        printf("4. Exit\n");
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number between 1 and 4.\n");
            // Clear input buffer
            while (getchar() != '\n');
            continue;
        }
        if (choice == 1) {
            print_all_accounts();
        } else if (choice == 2) {
            int acc_num;
            printf("Enter 6-digit account number: ");
            if (scanf("%d", &acc_num) != 1 || acc_num < 100000 || acc_num > 999999) {
                printf("Invalid account number format.\n");
                // Clear input buffer
                while (getchar() != '\n');
                continue;
            }
            fetch_transactions_by_account(acc_num);
        } else if (choice == 3) {
            int txn_id;
            printf("Enter transaction ID: ");
            if (scanf("%d", &txn_id) != 1 || txn_id <= 0 || txn_id > history_count) {
                printf("Invalid transaction ID format.\n");
                // Clear input buffer
                while (getchar() != '\n');
                continue;
            }
            fetch_transaction_by_id(txn_id);
        } else if (choice == 4) {
            printf("Exiting...\n");
            break;
        } else {
            printf("Invalid choice. Please select an option between 1 and 4.\n");
        }
    }

    return 0;
}
