#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define INF 1e9
#define BLOOM_FILTER_SIZE 1000
#define HASH_FUNCTIONS 3
#define MAX_NODES 100

// Structure to represent a transaction
typedef struct {
    char txn_id[7];  // 6-digit decimal transaction ID
    int src_node;
    int dest_node;
    double amount;
} Transaction;

// Structure to represent a node (account)
typedef struct {
    char name[50];  // Node's name (e.g., person's name)
    double balance;
    double fee_percentage;  // Fee percentage for this node (applied as an intermediate)
    Transaction* transactions[100];  // Array of transaction pointers (can hold up to 100 transactions)
    int transaction_count;
} Node;

// Array of nodes (representing the graph of accounts)
Node* account_graph[MAX_NODES];  // Supports up to 100 nodes
int node_count = 0;

// Bloom filter bit array for transaction IDs
int bloom_filter[BLOOM_FILTER_SIZE] = {0};

// Hash functions for the Bloom filter
int hash1(const char* key) {
    int hash = 0;
    while (*key) hash = (hash + *key++) % BLOOM_FILTER_SIZE;
    return hash;
}

int hash2(const char* key) {
    int hash = 0;
    while (*key) hash = (hash + *key++ * 7) % BLOOM_FILTER_SIZE;
    return hash;
}

int hash3(const char* key) {
    int hash = 0;
    while (*key) hash = (hash + *key++ * 11) % BLOOM_FILTER_SIZE;
    return hash;
}

// Shortest path matrix for final amount retained (maximize final amount received)
double max_final_amount[MAX_NODES][MAX_NODES];  // Matrix to hold the maximum final amount after fees
int next[MAX_NODES][MAX_NODES];  // To reconstruct the shortest path

// Function to generate a random 6-digit transaction ID
void generate_random_txn_id(char* txn_id) {
    sprintf(txn_id, "%06d", rand() % 900000 + 100000);
}

// Function to check if a transaction ID exists using the Bloom filter
int transaction_exists(const char* txn_id) {
    return bloom_filter[hash1(txn_id)] && bloom_filter[hash2(txn_id)] && bloom_filter[hash3(txn_id)];
}

// Function to add a transaction to the Bloom filter
void add_to_bloom_filter(const char* txn_id) {
    bloom_filter[hash1(txn_id)] = 1;
    bloom_filter[hash2(txn_id)] = 1;
    bloom_filter[hash3(txn_id)] = 1;
}

// Function to initialize the max final amount and path matrices for an undirected graph
void initialize_graph(int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                max_final_amount[i][j] = 1;  // 100% of money remains on the same node
            } else {
                max_final_amount[i][j] = 0;  // Start with 0 for unreachable nodes
            }
            next[i][j] = -1;
        }
    }
}

// Floyd-Warshall algorithm to compute the maximum amount received, accounting for node fees
void floyd_warshall_for_max_final_amount(int n) {
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                double new_amount = max_final_amount[i][k] * (1 - account_graph[k]->fee_percentage / 100.0) * max_final_amount[k][j];
                if (new_amount > max_final_amount[i][j]) {
                    max_final_amount[i][j] = new_amount;
                    next[i][j] = next[i][k];  // Update path
                }
            }
        }
    }
}

// Function to reconstruct the best path
void reconstruct_path(int src, int dest) {
    if (next[src][dest] == -1) {
        printf("No path exists from %d to %d\n", src, dest);
        return;
    }
    printf("Best path: ");
    int current = src;
    while (current != dest) {
        printf("%d -> ", current);
        current = next[current][dest];
    }
    printf("%d\n", dest);
}

// Perform transaction through the best path
void perform_transaction_via_path(int src, int dest, double amount) {
    if (max_final_amount[src][dest] == 0) {
        printf("No possible transaction path from %d to %d\n", src, dest);
        return;
    }

    printf("Starting transaction from node %d to node %d with amount %.2f\n", src, dest, amount);

    // Reconstruct the best path
    reconstruct_path(src, dest);

    // Perform the transaction by following the path and adjusting balances
    int current = src;
    while (current != dest) {
        int next_node = next[current][dest];
        double fee = amount * (account_graph[current]->fee_percentage / 100.0);
        double transferred_amount = amount - fee;

        // Adjust balances
        account_graph[current]->balance -= amount;
        account_graph[next_node]->balance += transferred_amount;

        printf("Node %d charges %.2f (%.2f%% fee), transfers %.2f to node %d\n", current, fee, account_graph[current]->fee_percentage, transferred_amount, next_node);

        amount = transferred_amount;  // Amount left to transfer in the next step
        current = next_node;
    }

    printf("Transaction complete. Final amount received by node %d: %.2f\n", dest, amount);
}

// Function to add a transaction to the graph
void add_transaction(const char* txn_id, int src, int dest, double amount) {
    if (amount <= 0) {
        printf("Invalid transaction amount.\n");
        return;
    }

    // Check if both src and dest nodes exist
    if (src >= node_count || dest >= node_count) {
        printf("Source or destination node does not exist.\n");
        return;
    }

    Node* src_node = account_graph[src];
    Node* dest_node = account_graph[dest];

    // Ensure that the source has enough balance
    if (src_node->balance < amount) {
        printf("Insufficient funds in source node.\n");
        return;
    }

    // Create a new transaction
    Transaction* txn = (Transaction*)malloc(sizeof(Transaction));
    strcpy(txn->txn_id, txn_id);
    txn->src_node = src;
    txn->dest_node = dest;
    txn->amount = amount;

    // Add the transaction to both source and destination nodes
    src_node->transactions[src_node->transaction_count++] = txn;
    dest_node->transactions[dest_node->transaction_count++] = txn;

    // Add the transaction ID to the Bloom filter
    add_to_bloom_filter(txn_id);

    printf("Transaction %s added successfully.\n", txn_id);
}

// Function to print the details of a transaction by its ID
void print_transaction_details(const char* txn_id) {
    if (!transaction_exists(txn_id)) {
        printf("Transaction ID %s not found in Bloom filter.\n", txn_id);
        return;
    }

    // Search for the transaction in the graph
    for (int i = 0; i < node_count; i++) {
        Node* node = account_graph[i];
        for (int j = 0; j < node->transaction_count; j++) {
            Transaction* txn = node->transactions[j];
            if (strcmp(txn->txn_id, txn_id) == 0) {
                printf("Transaction Found:\n");
                printf("  ID: %s\n", txn->txn_id);
                printf("  Source: %d (%s)\n", txn->src_node, account_graph[txn->src_node]->name);
                printf("  Destination: %d (%s)\n", txn->dest_node, account_graph[txn->dest_node]->name);
                printf("  Amount: %.2f\n", txn->amount);
                return;
            }
        }
    }

    printf("Transaction ID %s not found in graph.\n", txn_id);
}

// Function to add a new node to the system
void add_node(const char* name, double balance, double fee_percentage) {
    if (node_count >= MAX_NODES) {
        printf("Maximum node limit reached.\n");
        return;
    }

    // Create a new node and add it to the graph
    Node* node = (Node*)malloc(sizeof(Node));
    strcpy(node->name, name);
    node->balance = balance;
    node->fee_percentage = fee_percentage;
    node->transaction_count = 0;
    account_graph[node_count++] = node;

    printf("Node %s added successfully with balance: %.2f and fee percentage: %.2f%%\n", name, balance, fee_percentage);
}

// Function to read nodes from a file and initialize the account graph
void read_nodes_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file %s\n", filename);
        return;
    }

    char name[50];
    double balance, fee_percentage;

    while (fscanf(file, "%s %lf %lf", name, &balance, &fee_percentage) != EOF) {
        add_node(name, balance, fee_percentage);
    }

    fclose(file);
}

// Function to perform random transactions between nodes
void perform_random_transactions(int num_transactions) {
    for (int i = 0; i < num_transactions; i++) {
        char txn_id[7];
        generate_random_txn_id(txn_id);

        int src = rand() % node_count;
        int dest = rand() % node_count;
        double amount = (rand() % 1000) + 100;  // Random amount between 100 and 1000

        add_transaction(txn_id, src, dest, amount);
    }
}

// Function to print details of a specific node
void print_node_details(int node_index) {
    if (node_index >= node_count) {
        printf("Node index %d is out of range.\n", node_index);
        return;
    }

    Node* node = account_graph[node_index];
    printf("Node %d (%s):\n", node_index, node->name);
    printf("  Balance: %.2f\n", node->balance);
    printf("  Fee Percentage: %.2f%%\n", node->fee_percentage);
    printf("  Transaction Count: %d\n", node->transaction_count);

    for (int i = 0; i < node->transaction_count; i++) {
        Transaction* txn = node->transactions[i];
        printf("    Transaction %d:\n", i + 1);
        printf("      ID: %s\n", txn->txn_id);
        printf("      Source: %d (%s)\n", txn->src_node, account_graph[txn->src_node]->name);
        printf("      Destination: %d (%s)\n", txn->dest_node, account_graph[txn->dest_node]->name);
        printf("      Amount: %.2f\n", txn->amount);
    }
}

int main() {
    srand(time(NULL));
    //check if a state file exists
    FILE* file = fopen("state.txt", "r");
    if (file) {
        // Read the state from the file
        fscanf(file, "%d", &node_count);
        for (int i = 0; i < node_count; i++) {
            Node* node = (Node*)malloc(sizeof(Node));
            fscanf(file, "%s %lf %lf %d", node->name, &node->balance, &node->fee_percentage, &node->transaction_count);
            for (int j = 0; j < node->transaction_count; j++) {
                Transaction* txn = (Transaction*)malloc(sizeof(Transaction));
                fscanf(file, "%s %d %d %lf", txn->txn_id, &txn->src_node, &txn->dest_node, &txn->amount);
                node->transactions[j] = txn;
            }
            account_graph[i] = node;
        }
        fclose(file);
    } else {
        // Initialize the path matrices for undirected graph
        initialize_graph(node_count);

        // Read nodes from file
        read_nodes_from_file("accounts.txt");
    }
    // Read nodes from file
    // read_nodes_from_file("accounts.txt");

    // // Initialize the path matrices for undirected graph
    // initialize_graph(node_count);

    //read the trasactions from file trasaactions.txt and perform them
    file = fopen("transactions.txt", "r");
    if (!file) {
        printf("Error opening file transactions.txt\n");
        return 1;
    }
    //the file contains transaction in the format txn_id src dest amount
    char txn_id[7];
    int src, dest;
    double amount;
    while (fscanf(file, "%s %d %d %lf", txn_id, &src, &dest, &amount) != EOF) {
        add_transaction(txn_id, src, dest, amount);
    }
    fclose(file);


    // Calculate the best paths
    floyd_warshall_for_max_final_amount(node_count);

    // Perform transaction from node 0 to node 2 (minimizing the fees)
    perform_transaction_via_path(0, 2, 1000.0);  // Transfer $1000 from node 0 to node 2

    // Print details of 10 random nodes
    for (int i = 0; i < 10; i++) {
        int random_node = rand() % node_count;
        print_node_details(random_node);
    }
    // Save the state to a file
    file = fopen("state.txt", "w");
    if (!file) {
        printf("Error opening file state.txt\n");
        return 1;
    }
    fprintf(file, "%d\n", node_count);
    for (int i = 0; i < node_count; i++) {
        Node* node = account_graph[i];
        fprintf(file, "%s %.2f %.2f %d\n", node->name, node->balance, node->fee_percentage, node->transaction_count);
        for (int j = 0; j < node->transaction_count; j++) {
            Transaction* txn = node->transactions[j];
            fprintf(file, "%s %d %d %.2f\n", txn->txn_id, txn->src_node, txn->dest_node, txn->amount);
        }
    }
    fclose(file);


    return 0;
}
