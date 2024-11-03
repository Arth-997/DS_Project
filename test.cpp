#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <ctime>
#include <cmath>
#include <queue>
#include <functional>
#include <bitset>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <utility>
#include <limits> // For numeric_limits

const int BLOOM_FILTER_SIZE = 10000;  // Increased size for larger dataset
const int NUM_HASH_FUNCTIONS = 3;

// Transaction structure
struct Transaction {
    std::string transactionID;  // 6-digit transaction ID
    int senderAccountID;
    int receiverAccountID;
    double amount;
    long long timestamp;        // Simulated as epoch time or integer
    std::string description;    // Transaction description
};

// Account structure
struct Account {
    int accountID;
    double balance;
    std::vector<Transaction> transactionHistory;
};

// BK Tree Node
class BKTreeNode {
public:
    std::string word;
    std::unordered_map<int, BKTreeNode*> children;

    BKTreeNode(const std::string& w) : word(w) {}
};

// BK Tree for typo detection
class BKTree {
private:
    BKTreeNode* root;

    int levenshteinDistance(const std::string& s1, const std::string& s2) {
        int len1 = s1.size(), len2 = s2.size();
        std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

        for (int i = 0; i <= len1; ++i) d[i][0] = i;
        for (int j = 0; j <= len2; ++j) d[0][j] = j;

        for (int i = 1; i <= len1; ++i) {
            for (int j = 1; j <= len2; ++j) {
                int cost = (tolower(s1[i - 1]) == tolower(s2[j - 1])) ? 0 : 1;
                d[i][j] = std::min({
                    d[i - 1][j] + 1,       // Deletion
                    d[i][j - 1] + 1,       // Insertion
                    d[i - 1][j - 1] + cost // Substitution
                });
            }
        }
        return d[len1][len2];
    }

public:
    BKTree() : root(nullptr) {}

    ~BKTree() {
        // Destructor to clean up nodes
        if (root) {
            std::queue<BKTreeNode*> nodes;
            nodes.push(root);
            while (!nodes.empty()) {
                BKTreeNode* current = nodes.front();
                nodes.pop();
                for (auto& childPair : current->children) {
                    nodes.push(childPair.second);
                }
                delete current;
            }
        }
    }

    void insert(const std::string& word) {
        if (!root) {
            root = new BKTreeNode(word);
            return;
        }

        BKTreeNode* node = root;
        int distance = levenshteinDistance(word, node->word);

        while (node->children.count(distance)) {
            node = node->children[distance];
            distance = levenshteinDistance(word, node->word);
        }
        node->children[distance] = new BKTreeNode(word);
    }

    bool search(const std::string& query, int maxDistance) {
        if (!root) return false;

        std::queue<BKTreeNode*> nodes;
        nodes.push(root);

        while (!nodes.empty()) {
            BKTreeNode* node = nodes.front();
            nodes.pop();

            int distance = levenshteinDistance(query, node->word);
            if (distance <= maxDistance && distance > 0) {  // distance > 0 to exclude exact matches
                return true;
            }

            for (int i = distance - maxDistance; i <= distance + maxDistance; ++i) {
                if (i >= 0 && node->children.count(i)) {
                    nodes.push(node->children[i]);
                }
            }
        }
        return false;
    }
};

// Suffix Tree Node
class SuffixTreeNode {
public:
    std::unordered_map<char, SuffixTreeNode*> children;
    bool isEndOfWord;

    SuffixTreeNode() : isEndOfWord(false) {}
    
    // Destructor to delete children nodes
    ~SuffixTreeNode() {
        for (auto& pair : children) {
            delete pair.second; // Recursively delete all children
        }
    }
};

// Suffix Tree for pattern detection
class SuffixTree {
private:
    SuffixTreeNode* root;

public:
    SuffixTree() {
        root = new SuffixTreeNode();
    }

    ~SuffixTree() {
        delete root; // Delete the root and its children
    }

    void insert(const std::string& text) {
        for (size_t i = 0; i < text.length(); ++i) {
            SuffixTreeNode* node = root;
            for (size_t j = i; j < text.length(); ++j) {
                char c = tolower(text[j]);
                if (!node->children.count(c)) {
                    node->children[c] = new SuffixTreeNode();
                }
                node = node->children[c];
            }
            node->isEndOfWord = true;
        }
    }

    bool search(const std::string& pattern) {
        SuffixTreeNode* node = root;
        for (char c : pattern) {
            c = tolower(c);
            if (!node->children.count(c)) {
                return false;
            }
            node = node->children[c];
        }
        return node->isEndOfWord; // Check if it's the end of a word
    }

    // Clear the suffix tree
    void clear() {
        delete root; // Delete current root and all children
        root = new SuffixTreeNode(); // Reinitialize root
    }
};

// Bloom Filter for fast flagged account detection
class BloomFilter {
    std::bitset<BLOOM_FILTER_SIZE> filter;

    std::hash<int> hashFn;

    int hash(int data, int seed) {
        return (hashFn(data + seed)) % BLOOM_FILTER_SIZE;
    }

public:
    void insert(int accountID) {
        for (int i = 0; i < NUM_HASH_FUNCTIONS; ++i) {
            int index = hash(accountID, i);
            filter.set(index);
        }
    }

    bool possiblyExists(int accountID) {
        for (int i = 0; i < NUM_HASH_FUNCTIONS; ++i) {
            int index = hash(accountID, i);
            if (!filter.test(index))
                return false;
        }
        return true;
    }
};

// Fraud Detection System
class FraudDetectionSystem {
public:
    BKTree bkTree;
    SuffixTree suffixTree;
    BloomFilter bloomFilter;
    std::unordered_map<int, Account> accounts;
    std::unordered_map<std::string, Transaction> transactions;
    std::unordered_set<std::string> suspiciousPatterns;
    std::unordered_map<int, std::unordered_map<int, int>> transactionCounts; // For frequent transactions
    std::unordered_map<int, std::unordered_map<int, double>> transactionAmounts;
    std::unordered_map<int, std::vector<int>> graphAdjacencyList; // For graph representation

    FraudDetectionSystem() {}

    ~FraudDetectionSystem() {
        // Destructor to ensure all dynamically allocated memory is cleaned up
    }

    void addAccount(int accountID, double initialBalance) {
        if (accounts.find(accountID) != accounts.end()) {
            std::cout << "Account ID " << accountID << " already exists." << std::endl;
            return;
        }
        accounts[accountID] = { accountID, initialBalance, {} };
        std::cout << "Account ID " << accountID << " added with initial balance $" << initialBalance << "." << std::endl;
    }

    void bulkAddAccounts(int startID, int endID, double initialBalance) {
        for (int i = startID; i <= endID; ++i) {
            if (accounts.find(i) == accounts.end()) {
                accounts[i] = { i, initialBalance, {} };
                std::cout << "Account ID " << i << " added with initial balance $" << initialBalance << "." << std::endl;
            } else {
                std::cout << "Account ID " << i << " already exists. Skipping." << std::endl;
            }
        }
        std::cout << "Bulk account addition completed." << std::endl;
    }

    void processTransaction(const Transaction& tx) {
        // Check if sender and receiver exist
        if (accounts.find(tx.senderAccountID) == accounts.end() ||
            accounts.find(tx.receiverAccountID) == accounts.end()) {
            std::cout << "Invalid account involved in transaction ID: " << tx.transactionID << std::endl;
            return;
        }

        // Check if sender has enough balance
        if (accounts[tx.senderAccountID].balance < tx.amount) {
            std::cout << "Insufficient funds for transaction ID: " << tx.transactionID << std::endl;
            return;
        }

        // Check for flagged accounts
        if (bloomFilter.possiblyExists(tx.senderAccountID) || bloomFilter.possiblyExists(tx.receiverAccountID)) {
            std::cout << "Alert: Flagged account involved in transaction ID: " << tx.transactionID
                      << " (Reason: Flagged Account)" << std::endl;
            return;  // Transaction fails
        }

        bool isFraudulent = false;
        std::string fraudReason;

        // Check for suspicious description using BK Tree (typosquatting)
        std::istringstream iss(tx.description);
        std::string word;
        while (iss >> word) {
            if (bkTree.search(word, 2)) {  // Levenshtein distance <= 2
                isFraudulent = true;
                fraudReason = "Suspicious word detected: '" + word + "'";
                break;
            }
        }

        // Check for suspicious patterns using Suffix Tree
        if (!isFraudulent) {
            // Insert the transaction description into the suffix tree
            suffixTree.insert(tx.description);
            
            // Check for suspicious patterns
            for (const auto& pattern : suspiciousPatterns) {
                if (suffixTree.search(pattern)) {
                    isFraudulent = true;  // Mark as fraudulent
                    fraudReason = "Suspicious pattern detected: '" + pattern + "'";
                    break;  // Exit the loop on first detection
                }
            }
            
            // Clear the suffix tree after processing the transaction
            suffixTree.clear();
        }

        // Velocity Fraud Detection
        if (!isFraudulent && detectVelocityFraud(tx.senderAccountID, tx.timestamp)) {
            isFraudulent = true;
            fraudReason = "Velocity fraud detected";
        }

        // Frequent Transactions to Same Account
        if (!isFraudulent && detectFrequentTransactions(tx.senderAccountID, tx.receiverAccountID, tx.amount)) {
            isFraudulent = true;
            fraudReason = "Frequent large transactions to the same account";
        }

        // Circular Transactions Detection
        // Add the edge to the graph
        graphAdjacencyList[tx.senderAccountID].push_back(tx.receiverAccountID);
        if (!isFraudulent && detectCircularTransactions(tx.senderAccountID, tx.receiverAccountID)) {
            isFraudulent = true;
            // Remove the sender and receiver from the adjacency list
            graphAdjacencyList[tx.senderAccountID].pop_back();
            fraudReason = "Circular transactions detected";
        }

        if (isFraudulent) {
            // Flag the account
            bloomFilter.insert(tx.senderAccountID);
            std::cout << "Alert: Transaction ID " << tx.transactionID << " failed. Reason: " << fraudReason << std::endl;
            std::cout << "Account ID " << tx.senderAccountID << " has been flagged." << std::endl;
            return;  // Transaction fails
        }

        // Process the transaction
        accounts[tx.senderAccountID].balance -= tx.amount;
        accounts[tx.receiverAccountID].balance += tx.amount;

        // Add transaction to histories
        accounts[tx.senderAccountID].transactionHistory.push_back(tx);
        accounts[tx.receiverAccountID].transactionHistory.push_back(tx);
        transactions[tx.transactionID] = tx;

        // Update transaction counts and amounts
        transactionCounts[tx.senderAccountID][tx.receiverAccountID]++;
        transactionAmounts[tx.senderAccountID][tx.receiverAccountID] += tx.amount;

        // Update graph
        graphAdjacencyList[tx.senderAccountID].push_back(tx.receiverAccountID);

        std::cout << "Transaction ID " << tx.transactionID << " processed successfully." << std::endl;
    }

    // Velocity Fraud Detection
    bool detectVelocityFraud(int accountID, long long currentTimestamp) {
        const int TIME_WINDOW = 60;  // 60 seconds
        const int MAX_TRANSACTIONS = 5;  // Adjusted for larger dataset

        auto& history = accounts[accountID].transactionHistory;
        int transactionCount = 0;

        // Count transactions within the time window
        for (auto it = history.rbegin(); it != history.rend(); ++it) {
            if (currentTimestamp - it->timestamp <= TIME_WINDOW) {
                transactionCount++;
                if (transactionCount >= MAX_TRANSACTIONS) {
                    return true;
                }
            } else {
                break;
            }
        }
        return false;
    }

    // Frequent Transactions to the Same Account
    bool detectFrequentTransactions(int senderID, int receiverID, double amount) {
        const int TRANSACTION_THRESHOLD = 3;
        const double AMOUNT_THRESHOLD = 50000.0;  // Example threshold

        int count = transactionCounts[senderID][receiverID] + 1;
        double totalAmount = transactionAmounts[senderID][receiverID] + amount;

        if (count >= TRANSACTION_THRESHOLD && totalAmount >= AMOUNT_THRESHOLD) {
            return true;
        }
        return false;
    }

    // Circular Transactions Detection
    bool detectCircularTransactions(int senderID, int receiverID) {
        std::unordered_set<int> visited;
        return isCyclic(senderID, senderID, visited, 0);
    }

    bool isCyclic(int currentID, int targetID, std::unordered_set<int>& visited, int depth) {
        if (depth > 10) return false;  // Limit depth to prevent deep recursion
        visited.insert(currentID);

        for (int neighbor : graphAdjacencyList[currentID]) {
            if (neighbor == targetID && depth > 0) {
                return true;
            }
            if (!visited.count(neighbor)) {
                if (isCyclic(neighbor, targetID, visited, depth + 1)) {
                    return true;
                }
            }
        }
        visited.erase(currentID);
        return false;
    }

    // Function to retrieve a transaction by ID
    Transaction* getTransaction(const std::string& transactionID) {
        if (transactions.find(transactionID) != transactions.end()) {
            return &transactions[transactionID];
        }
        return nullptr;
    }

    // Function to print account balance
    void printAccountBalance(int accountID) {
        if (accounts.find(accountID) != accounts.end()) {
            std::cout << "Account ID: " << accountID << ", Balance: $" << accounts[accountID].balance << std::endl;
        } else {
            std::cout << "Account ID: " << accountID << " does not exist." << std::endl;
        }
    }

    // Function to load suspicious patterns
    void addSuspiciousPattern(const std::string& pattern) {
        suspiciousPatterns.insert(pattern);
    }

    // Function to display all accounts
    void displayAllAccounts() {
        if (accounts.empty()) {
            std::cout << "No accounts available." << std::endl;
            return;
        }
        std::cout << "List of Accounts:" << std::endl;
        for (const auto& pair : accounts) {
            std::cout << "Account ID: " << pair.second.accountID << ", Balance: $" << pair.second.balance << std::endl;
        }
    }

    // Function to display all transactions
    void displayAllTransactions() {
        if (transactions.empty()) {
            std::cout << "No transactions available." << std::endl;
            return;
        }
        std::cout << "List of Transactions:" << std::endl;
        for (const auto& pair : transactions) {
            const Transaction& tx = pair.second;
            std::cout << "Transaction ID: " << tx.transactionID
                      << ", Sender: " << tx.senderAccountID
                      << ", Receiver: " << tx.receiverAccountID
                      << ", Amount: $" << tx.amount
                      << ", Timestamp: " << tx.timestamp
                      << ", Description: " << tx.description << std::endl;
        }
    }
};

// Function to read words from a file and insert into BKTree
void loadWordsIntoBKTree(const std::string& filename, BKTree& bkTree) {
    std::ifstream file(filename);
    std::string word;
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    while (file >> word) {
        // Insert into BK Tree
        bkTree.insert(word);
    }
    file.close();
}

// Function to read words from a file and add to suspicious patterns
void loadWordsIntoSuffixTree(const std::string& filename, SuffixTree& suffixTree, std::unordered_set<std::string>& suspiciousPatterns) {
    std::ifstream file(filename);
    std::string word;
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    while (file >> word) {
        // Add suspicious words to the set
        suspiciousPatterns.insert(word);
    }
    file.close();
}

// Function to load transactions from a file
std::vector<Transaction> loadTransactionsFromFile(const std::string& filename) {
    std::vector<Transaction> transactions;
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error opening file for reading: " << filename << std::endl;
        return transactions;
    }

    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        Transaction tx;
        std::string transactionID;
        std::string sender, receiver, amount, timestamp, description;

        // Parse the line
        if (std::getline(iss, transactionID, ',') &&
            std::getline(iss, sender, ',') &&
            std::getline(iss, receiver, ',') &&
            std::getline(iss, amount, ',') &&
            std::getline(iss, timestamp, ',') &&
            std::getline(iss, description)) {
                
            tx.transactionID = transactionID;
            tx.senderAccountID = std::stoi(sender);
            tx.receiverAccountID = std::stoi(receiver);
            tx.amount = std::stod(amount);
            tx.timestamp = std::stoll(timestamp);
            tx.description = description;

            transactions.push_back(tx);
        }
    }

    infile.close();
    return transactions;
}

// Function to display the menu
void displayMenu() {
    std::cout << "\n=== Fraud Detection System Menu ===\n";
    std::cout << "1. Load BK Tree Words\n";
    std::cout << "2. Load Suffix Tree Words\n";
    std::cout << "3. Add Single Account\n";
    std::cout << "4. Add Multiple Accounts\n";
    std::cout << "5. Load Transactions from File\n";
    std::cout << "6. Process Transactions\n";
    std::cout << "7. Display All Accounts\n";
    std::cout << "8. Display All Transactions\n";
    std::cout << "9. Exit\n";
    std::cout << "Please select an option (1-9): ";
}

int main() {
    FraudDetectionSystem fds;
    bool exitProgram = false;

    while (!exitProgram) {
        displayMenu();
        int choice;
        std::cin >> choice;

        // Clear input buffer to handle any extra input
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: {
                // Load BK Tree Words
                std::string filename;
                std::cout << "Enter the filename for BK Tree words (e.g., bk_tree_words.txt): ";
                std::getline(std::cin, filename);
                loadWordsIntoBKTree(filename, fds.bkTree);
                std::cout << "BK Tree words loaded successfully from " << filename << "." << std::endl;
                break;
            }
            case 2: {
                // Load Suffix Tree Words
                std::string filename;
                std::cout << "Enter the filename for Suffix Tree suspicious patterns (e.g., suffix_tree_words.txt): ";
                std::getline(std::cin, filename);
                loadWordsIntoSuffixTree(filename, fds.suffixTree, fds.suspiciousPatterns);
                std::cout << "Suffix Tree suspicious patterns loaded successfully from " << filename << "." << std::endl;
                break;
            }
            case 3: {
                // Add Single Account
                int accountID;
                double initialBalance;
                std::cout << "Enter Account ID (integer): ";
                std::cin >> accountID;
                std::cout << "Enter Initial Balance (e.g., 100000.0): ";
                std::cin >> initialBalance;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear buffer
                fds.addAccount(accountID, initialBalance);
                break;
            }
            case 4: {
                // Add Multiple Accounts
                int startID, endID;
                double initialBalance;
                std::cout << "Enter starting Account ID (integer): ";
                std::cin >> startID;
                std::cout << "Enter ending Account ID (integer): ";
                std::cin >> endID;
                std::cout << "Enter Initial Balance for each account (e.g., 100000.0): ";
                std::cin >> initialBalance;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear buffer
                if (startID > endID) {
                    std::cout << "Starting Account ID should be less than or equal to Ending Account ID." << std::endl;
                } else {
                    fds.bulkAddAccounts(startID, endID, initialBalance);
                }
                break;
            }
            case 5: {
                // Load Transactions from File
                std::string filename;
                std::cout << "Enter the filename for initial transactions (e.g., initial_transactions.txt): ";
                std::getline(std::cin, filename);
                std::vector<Transaction> transactions = loadTransactionsFromFile(filename);
                if (!transactions.empty()) {
                    for (const auto& tx : transactions) {
                        fds.processTransaction(tx);
                    }
                    std::cout << "Transactions loaded and processed successfully from " << filename << "." << std::endl;
                } else {
                    std::cout << "No transactions to process from " << filename << "." << std::endl;
                }
                break;
            }
            case 6: {
                // Process Transactions
                std::string filename;
                std::cout << "Enter the filename for transactions to process (e.g., new_transactions.txt): ";
                std::getline(std::cin, filename);
                std::vector<Transaction> transactions = loadTransactionsFromFile(filename);
                if (!transactions.empty()) {
                    for (const auto& tx : transactions) {
                        fds.processTransaction(tx);
                    }
                    std::cout << "Transactions processed successfully from " << filename << "." << std::endl;
                } else {
                    std::cout << "No transactions to process from " << filename << "." << std::endl;
                }
                break;
            }
            case 7: {
                // Display All Accounts
                fds.displayAllAccounts();
                break;
            }
            case 8: {
                // Display All Transactions
                fds.displayAllTransactions();
                break;
            }
            case 9: {
                // Exit
                std::cout << "Exiting the Fraud Detection System. Goodbye!" << std::endl;
                exitProgram = true;
                break;
            }
            default: {
                std::cout << "Invalid option selected. Please try again." << std::endl;
                break;
            }
        }
    }

    return 0;
}
