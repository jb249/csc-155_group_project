// CSC-155 Group Programming Assignment

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <limits>

using namespace std;

// ------------------ Account Structure ------------------
struct Account {
    string accountNumber;   // preserve leading zeros
    string name;
    double balance;
    string passwordHash;
    bool locked;
};

// ------------------ Function Prototypes ------------------
void createAccount();
void deposit();
void withdraw();
void checkBalance();
void logTransaction(const string &accNum, const string &type, double amount, double newBalance);
void viewTransactions();
void displayAllAccounts();
void unlockAccount();
bool loadAccounts(vector<Account> &accounts);
void saveAccounts(const vector<Account> &accounts);
string hashPassword(const string &password);
bool verifyPassword(Account &acc);

// Admin functions
bool loadAdmin(string &adminHash);
bool saveAdmin(const string &adminHash);
bool ensureAdminExists();
bool verifyAdmin();

const string ADMIN_FILENAME = "admin.txt";

// ------------------ Main Menu ------------------
int main() {
    if (!ensureAdminExists()) {
        cout << "ERROR: Failed to initialize admin credentials. Exiting.\n";
        return 1;
    }

    int choice;
    do {
        cout << "\n===== Banking System =====\n";
        cout << "1. Create Account\n2. Deposit\n3. Withdraw\n4. Check Balance\n5. Exit\n";
        cout << "6. View Transaction History\n7. Display All Accounts (Admin)\n8. Unlock Account (Admin)\n";
        cout << "Enter your choice: ";
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "ERROR: Invalid input. Please enter a number.\n";
            continue;
        }

        switch(choice) {
            case 1: createAccount(); break;
            case 2: deposit(); break;
            case 3: withdraw(); break;
            case 4: checkBalance(); break;
            case 5: cout << "Exiting program... Goodbye!\n"; break;
            case 6: viewTransactions(); break;
            case 7:
                if (verifyAdmin()) displayAllAccounts();
                else cout << "ERROR: Admin authentication failed.\n";
                break;
            case 8:
                if (verifyAdmin()) unlockAccount();
                else cout << "ERROR: Admin authentication failed.\n";
                break;
            default: cout << "ERROR: Invalid choice. Please try again.\n";
        }
    } while(choice != 5);

    return 0;
}

// ------------------ Helper Functions ------------------

string hashPassword(const string &password) {
    unsigned long hash = 5381;
    for (size_t i = 0; i < password.size(); i++) {
        char c = password[i];
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
    }
    return to_string(hash);
}

// ------------------ Admin Helpers ------------------
bool loadAdmin(string &adminHash) {
    ifstream in(ADMIN_FILENAME);
    if (!in) return false;
    if (!getline(in, adminHash)) return false;
    in.close();
    return !adminHash.empty();
}

bool saveAdmin(const string &adminHash) {
    ofstream out(ADMIN_FILENAME);
    if (!out) return false;
    out << adminHash << '\n';
    out.close();
    return true;
}

bool ensureAdminExists() {
    string adminHash;
    if (loadAdmin(adminHash)) return true;

    cout << "No admin/master password found. Please create one now.\n";
    string pass1, pass2;
    cout << "Enter new master password (max 20 characters): ";
    cin >> pass1;
    cout << "Confirm master password: ";
    cin >> pass2;

    if (pass1 != pass2) {
        cout << "ERROR: Passwords do not match. Please rerun the program to set admin password.\n";
        return false;
    }
    if (pass1.empty() || pass1.size() > 20) {
        cout << "ERROR: Invalid password length.\n";
        return false;
    }

    adminHash = hashPassword(pass1);
    if (!saveAdmin(adminHash)) {
        cout << "ERROR: Could not save admin credentials.\n";
        return false;
    }
    cout << "SUCCESS: Master password created.\n";
    return true;
}

bool verifyAdmin() {
    string adminHash;
    if (!loadAdmin(adminHash)) {
        cout << "ERROR: Admin file missing or unreadable.\n";
        return false;
    }

    int attempts = 0;
    string input;
    while (attempts < 3) {
        cout << "Enter master password: ";
        cin >> input;
        if (hashPassword(input) == adminHash) return true;
        attempts++;
        cout << "ERROR: Incorrect master password. Attempts left: " << (3 - attempts) << endl;
    }
    return false;
}

// ------------------ Account File IO ------------------
bool loadAccounts(vector<Account> &accounts) {
    ifstream inFile("accounts.txt");
    if (!inFile) return false;

    accounts.clear();
    string line;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        Account acc;
        acc.locked = false;

        size_t quote1 = line.find('"');
        size_t quote2 = (quote1 == string::npos) ? string::npos : line.find('"', quote1 + 1);
        if (quote1 == string::npos || quote2 == string::npos) continue;

        string beforeName = line.substr(0, quote1);
        while (!beforeName.empty() && isspace(beforeName.back())) beforeName.pop_back();
        acc.accountNumber = beforeName;

        acc.name = line.substr(quote1 + 1, quote2 - quote1 - 1);

        string rest = line.substr(quote2 + 1);
        istringstream iss(rest);
        iss >> acc.balance >> acc.passwordHash >> acc.locked;

        accounts.push_back(acc);
    }
    inFile.close();
    return true;
}

void saveAccounts(const vector<Account> &accounts) {
    ofstream outFile("accounts.txt");
    if (!outFile) {
        cout << "ERROR: Cannot write to accounts file.\n";
        return;
    }
    for (size_t i = 0; i < accounts.size(); i++) {
        outFile << accounts[i].accountNumber << " \"" << accounts[i].name << "\" "
                << accounts[i].balance << " " << accounts[i].passwordHash << " "
                << accounts[i].locked << endl;
    }
}

bool verifyPassword(Account &acc) {
    if (acc.locked) {
        cout << "ERROR: Account is locked.\n";
        return false;
    }

    int attempts = 0;
    string inputPass, hashedInput;
    while (attempts < 3) {
        cout << "Enter password: ";
        cin >> inputPass;
        hashedInput = hashPassword(inputPass);
        if (hashedInput == acc.passwordHash) return true;
        attempts++;
        cout << "ERROR: Incorrect password. Attempts left: " << (3 - attempts) << endl;
    }
    cout << "ERROR: Account locked due to too many failed attempts.\n";
    acc.locked = true;
    return false;
}

// ------------------ Core Banking Features ------------------
void createAccount() {
    vector<Account> accounts;
    loadAccounts(accounts);

    Account acc;
    cout << "\n--- Create New Account ---\n";
    cout << "Enter account number: ";
    cin >> acc.accountNumber;

    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accountNumber == acc.accountNumber) {
            cout << "ERROR: Account number already exists.\n";
            return;
        }
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Enter full name: ";
    getline(cin, acc.name);

    cout << "Enter initial balance: ";
    if (!(cin >> acc.balance)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "ERROR: Invalid balance input.\n";
        return;
    }
    if (acc.balance < 0) {
        cout << "ERROR: Balance cannot be negative.\n";
        return;
    }

    cout << "Set a password (max 12 chars): ";
    string password;
    cin >> password;
    if (password.length() > 12) {
        cout << "ERROR: Password too long.\n";
        return;
    }
    acc.passwordHash = hashPassword(password);
    acc.locked = false;

    accounts.push_back(acc);
    saveAccounts(accounts);

    cout << "SUCCESS: Account created.\n";
    logTransaction(acc.accountNumber, "Account Created", acc.balance, acc.balance);
}

void checkBalance() {
    vector<Account> accounts;
    if (!loadAccounts(accounts)) { cout << "ERROR: No accounts found.\n"; return; }

    string accNum;
    cout << "\nEnter account number: ";
    cin >> accNum;

    bool found = false;
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accountNumber == accNum) {
            if (!verifyPassword(accounts[i])) { saveAccounts(accounts); return; }
            cout << "\n--- Account Details ---\n";
            cout << "Account Number: " << accounts[i].accountNumber << "\nName: " << accounts[i].name
                 << "\nBalance: $" << accounts[i].balance << endl;
            found = true;
            break;
        }
    }
    if (!found) cout << "ERROR: Account not found.\n";
    saveAccounts(accounts);
}

void deposit() {
    vector<Account> accounts;
    if (!loadAccounts(accounts)) { cout << "ERROR: No accounts found.\n"; return; }

    string accNum;
    cout << "\nEnter account number: ";
    cin >> accNum;

    double amount;
    cout << "Enter amount to deposit: ";
    cin >> amount;
    if (amount <= 0) { cout << "Deposit cancelled.\n"; return; }

    bool found = false;
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accountNumber == accNum) {
            if (!verifyPassword(accounts[i])) { saveAccounts(accounts); return; }
            accounts[i].balance += amount;
            cout << "SUCCESS: Deposit successful. New balance: $" << accounts[i].balance << endl;
            logTransaction(accounts[i].accountNumber, "Deposit", amount, accounts[i].balance);
            found = true;
            break;
        }
    }
    if (!found) cout << "ERROR: Account not found.\n";
    saveAccounts(accounts);
}

void withdraw() {
    vector<Account> accounts;
    if (!loadAccounts(accounts)) { cout << "ERROR: No accounts found.\n"; return; }

    string accNum;
    cout << "\nEnter account number: ";
    cin >> accNum;

    double amount;
    cout << "Enter amount to withdraw (0 to cancel): ";
    cin >> amount;

    if (amount <= 0) { cout << "Withdrawal cancelled.\n"; return; }

    bool found = false;
    const double EPSILON = 1e-6;

    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accountNumber == accNum) {
            if (!verifyPassword(accounts[i])) { saveAccounts(accounts); return; }

            if (accounts[i].balance + EPSILON >= amount) {
                accounts[i].balance -= amount;
                if (accounts[i].balance < EPSILON) accounts[i].balance = 0;
                cout << "SUCCESS: Withdrawal successful. New balance: $" << accounts[i].balance << endl;
                logTransaction(accounts[i].accountNumber, "Withdraw", amount, accounts[i].balance);
            } else {
                cout << "ERROR: Insufficient funds. Current balance: $" << accounts[i].balance << endl;
            }
            found = true;
            break;
        }
    }
    if (!found) cout << "ERROR: Account not found.\n";
    saveAccounts(accounts);
}

// ------------------ Logging ------------------
void logTransaction(const string &accNum, const string &type, double amount, double newBalance) {
    ofstream logFile("transactions.txt", ios::app);
    if (!logFile) { cout << "ERROR: Cannot open transaction file.\n"; return; }

    time_t now = time(0);
    char* dt = ctime(&now);
    if (dt) dt[strlen(dt)-1] = '\0';

    logFile << "[" << (dt ? dt : "Unknown time") << "] Account " << accNum
            << " - " << type << " $" << amount << " | New Balance: $" << newBalance << endl;
}

void viewTransactions() {
    ifstream logFile("transactions.txt");
    if (!logFile) { cout << "ERROR: No transaction history found.\n"; return; }

    cout << "\n--- Transaction History ---\n";
    string line;
    while (getline(logFile, line)) cout << line << endl;
}

// ------------------ Admin ------------------
void displayAllAccounts() {
    vector<Account> accounts;
    if (!loadAccounts(accounts)) { cout << "ERROR: No accounts found.\n"; return; }

    sort(accounts.begin(), accounts.end(), [](const Account &a, const Account &b) {
        return a.accountNumber < b.accountNumber;
    });

    cout << "\n--- All Accounts ---\n";
    for (size_t i = 0; i < accounts.size(); i++)
        cout << "Number: " << accounts[i].accountNumber
             << " | Name: " << accounts[i].name
             << " | Balance: $" << accounts[i].balance
             << " | Locked: " << (accounts[i].locked ? "Yes" : "No") << endl;
}

void unlockAccount() {
    vector<Account> accounts;
    if (!loadAccounts(accounts)) { cout << "ERROR: No accounts found.\n"; return; }

    string accNum;
    cout << "Enter account number to unlock: ";
    cin >> accNum;

    bool found = false;
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accountNumber == accNum) {
            accounts[i].locked = false;
            cout << "SUCCESS: Account unlocked.\n";
            found = true;
            break;
        }
    }
    if (!found) cout << "ERROR: Account not found.\n";
    saveAccounts(accounts);
}
