//! Main File: backup.cpp

#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstdlib>

namespace fs = std::filesystem;
using namespace std;

//* function to get a timestamp
string getTimestamp() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm localTime;
    localtime_s(&localTime, &t);
    
    stringstream ss;
    ss << put_time(&localTime, "%Y-%m-%d_%H-%M-%S");
    return ss.str();
}

//* function to get current username (cross-platform)
string getCurrentUser() {
#ifdef _WIN32
    char* user = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&user, &len, "USERNAME");
    string result = (err == 0 && user) ? string(user) : "unknown";
    if (user) free(user);
    return result;
#else
    const char* user = getenv("USER");
    return user ? string(user) : "unknown";
#endif
}

//* function to get current working directory
string getCurrentDir() {
    return fs::current_path().string();
}

//* function to initialize backup system
void initBackup() {
    fs::create_directories(".backup");
    // Write metadata to .backup/__init__
    ofstream metaFile(".backup/__init__");
    metaFile << "init: True" << endl;
    metaFile << "author: " << getCurrentUser() << endl;
    metaFile << "folder: " << getCurrentDir() << endl;
    metaFile << "timestamp: " << getTimestamp() << endl;
    metaFile.close();
    cout << "Backup system initialized in `.backup/` folder." << endl;
}

//* function to check if backup is initialized
bool isBackupInitialized() {
    ifstream metaFile(".backup/__init__");
    if (!metaFile) return false;
    string line;
    while (getline(metaFile, line)) {
        if (line.find("init: True") != string::npos) {
            return true;
        }
    }
    return false;
}

//* function to show backup meta info
void showBackupMeta() {
    string metaPath = ".backup/__init__";
    ifstream metaFile(metaPath);
    if (!metaFile) {
        cout << "No backup meta found. Please run `backup init` first." << endl;
        return;
    }
    cout << "Backup Meta Information:" << endl;
    string line;
    while (getline(metaFile, line)) {
        cout << "  " << line << endl;
    }
    metaFile.close();
}

//* function to create a backup
void createBackup() {
    string backupDir = ".backup/Backup_" + getTimestamp();
    fs::create_directories(backupDir);

    for (const auto& file : fs::directory_iterator(".")) {
        if (file.path().filename() != "backup.cpp" && file.path().filename() != ".backup") {
            fs::copy(file.path(), backupDir + "/" + file.path().filename().string(),
                     fs::copy_options::overwrite_existing);
        }
    }

    cout << "Backup saved to: " << backupDir << endl;
}

//* function for automatic backups
void autoBackup(int minutes) {
    while (true) {
        createBackup();
        cout << "Waiting " << minutes << " minutes for the next backup..." << endl;
        this_thread::sleep_for(chrono::minutes(minutes));
    }
}

//* function to remove all backups
void removeAllBackups() {
    fs::remove_all(".backup");
    cout << "All backups removed." << endl;
}

void removeBackupCommand() {
    //* future implementation
    cout << "Backup command unregistered." << endl;
}

//* function to get sorted backup directories
vector<fs::directory_entry> getSortedBackups() {
    vector<fs::directory_entry> backups;
    if (fs::exists(".backup") && fs::is_directory(".backup")) {
        for (const auto& entry : fs::directory_iterator(".backup")) {
            if (fs::is_directory(entry)) {
                backups.push_back(entry);
            }
        }
        sort(backups.begin(), backups.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
            return a.path().filename().string() > b.path().filename().string(); // newest first
        });
    }
    return backups;
}

//* function to restore from a backup directory
void restoreBackup(const fs::path& backupDir) {
    for (const auto& file : fs::directory_iterator(backupDir)) {
        fs::copy(file.path(), "./" + file.path().filename().string(), fs::copy_options::overwrite_existing);
    }
    cout << "Restored from backup: " << backupDir << endl;
}

//* function to pull last backup
void pullLastBackup() {
    auto backups = getSortedBackups();
    if (!backups.empty()) {
        restoreBackup(backups[0].path());
    } else {
        cout << "No backups found." << endl;
    }
}

//* function to pull specific backup by index
void pullSpecificBackup(int index) {
    auto backups = getSortedBackups();
    if (index >= 0 && index < backups.size()) {
        restoreBackup(backups[index].path());
    } else {
        cout << "Backup index out of range." << endl;
    }
}

//* function to show help menu
void showHelp() {
    cout << "Backup Manager Commands:\n";
    cout << "  backup init            -> Initialize backup system\n";
    cout << "  backup do              -> Create a new backup\n";
    cout << "  backup auto --min X    -> Auto backup every X minutes\n";
    cout << "  backup remove --all    -> Delete all backups\n";
    cout << "  backup pull --last     -> Restore from the last backup\n";
    cout << "  backup pull --specific N -> Restore from the Nth last backup (0 = last, 1 = second last, etc)\n";
    cout << "  backup meta            -> Show backup meta information\n";
    cout << "  backup help            -> Show available commands\n";
    cout << "  backup remove-command  -> Unregister backup command\n";
}

//* main function
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: backup <command/help>\n";
        return 1;
    }

    string command = argv[1];

    // Only allow these commands before init
    if (command != "init" && command != "meta" && command != "help") {
        if (!isBackupInitialized()) {
            cout << "Backup system not initialized. Please run `backup init` first." << endl;
            return 1;
        }
    }

    if (command == "init") {
        initBackup();
    } else if (command == "do") {
        createBackup();
    } else if (command == "auto" && argc == 4 && string(argv[2]) == "--min") {
        int minutes = stoi(argv[3]);
        autoBackup(minutes);
    } else if (command == "remove" && argc == 3 && string(argv[2]) == "--all") {
        removeAllBackups();
    } else if (command == "help") {
        showHelp();
    } else if (command == "pull" && argc == 3 && string(argv[2]) == "--last") {
        pullLastBackup();
    } else if (command == "pull" && argc == 4 && string(argv[2]) == "--specific") {
        int idx = stoi(argv[3]);
        pullSpecificBackup(idx);
    } else if (command == "meta") {
        showBackupMeta();
    } else {
        cout << "Invalid command. Use `backup help` for options.\n";
    }

    return 0;
}
