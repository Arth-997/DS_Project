#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>

// Transaction structure
struct Transaction {
    std::string transactionID;
    int senderAccountID;
    int receiverAccountID;
    double amount;
    long long timestamp;
    std::string description;
};

// Function to generate the transaction records file
void generateTransactionFile(const std::string& filename, int numberOfTransactions) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error opening file for writing." << std::endl;
        return;
    }

    srand(time(0)); // Seed random number generator

    // Generate transactions
    for (int i = 0; i < numberOfTransactions; ++i) {
        int sender = 100 + rand() % 100; // Random sender ID between 100 and 119
        int receiver = 100 + rand() % 100; // Random receiver ID between 100 and 119
        while (receiver == sender) {
            receiver = 100 + rand() % 100; // Ensure sender and receiver are different
        }

        double amount = (rand() % 1000) + 100; // Amount between 100 and 1100
        std::string transactionID = std::to_string(i + 1);
        while (transactionID.length() < 6) transactionID = "0" + transactionID; // Pad transaction ID

        std::vector<std::string> descriptions = {
            "Amazon purchase", "Groceries", "Electronics", "Salary", "Rent payment",
            "Utility bill", "Insurance premium", "Mortgage payment", "Subscription fee",
            "Refund", "Invoice payment", "Deposit", "Withdrawal", "Transfer", "SALE offer",
            "DISCOUNT deal", "FREE gift", "OFFER limited", "PRIZE winner", "WINNER announcement",
            "Amaz0n", "Ebayy", "G00gle", "Micro$oft", "Faceb00k"
        };
        std::string description = descriptions[rand() % descriptions.size()];

        // Write transaction to file
        outfile << transactionID << "," << sender << "," << receiver << "," << amount 
                << "," << (time(0) + i) << "," << description << "\n"; // Use current time + i for unique timestamps
    }

    outfile.close();
    std::cout << "Transaction file generated: " << filename << std::endl;
}

// Usage example
int main() {
    generateTransactionFile("initial_transactions.txt", 200); // Generate 20 transactions
    return 0;
}
