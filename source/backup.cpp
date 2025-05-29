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
    try {
        fs::create_directories(".backup");
        ofstream metaFile(".backup/__init__");
        if (!metaFile) throw runtime_error("Failed to create metadata file.");
        
        metaFile << "init: True" << endl;
        metaFile << "author: " << getCurrentUser() << endl;
        metaFile << "folder: " << getCurrentDir() << endl;
        metaFile << "timestamp: " << getTimestamp() << endl;
        metaFile.close();
        cout << "Backup system initialized in `.backup/` folder." << endl;
    } catch (const exception& e) {
        cerr << "Error initializing backup: " << e.what() << endl;
    }
}

//* function to check if backup is initialized
bool isBackupInitialized() {
    try {
        ifstream metaFile(".backup/__init__");
        if (!metaFile) return false;
        string line;
        while (getline(metaFile, line)) {
            if (line.find("init: True") != string::npos) return true;
        }
    } catch (...) {
        return false;
    }
    return false;
}

//* function to create a backup safely
void createBackup() {
    try {
        string backupDir = ".backup/Backup_" + getTimestamp();
        fs::create_directories(backupDir);

        for (const auto& file : fs::directory_iterator(".")) {
            string filename = file.path().filename().string();
            if (filename != "backup.cpp" && filename != ".backup") {
                fs::copy(file.path(), backupDir + "/" + filename, fs::copy_options::overwrite_existing);
            }
        }
        cout << "Backup saved to: " << backupDir << endl;
    } catch (const exception& e) {
        cerr << "Error creating backup: " << e.what() << endl;
    }
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
    try {
        fs::remove_all(".backup");
        cout << "All backups removed." << endl;
    } catch (const exception& e) {
        cerr << "Error removing backups: " << e.what() << endl;
    }
}

//* function to restore from a backup directory
void restoreBackup(const fs::path& backupDir) {
    try {
        for (const auto& file : fs::directory_iterator(backupDir)) {
            fs::copy(file.path(), "./" + file.path().filename().string(), fs::copy_options::overwrite_existing);
        }
        cout << "Restored from backup: " << backupDir.string() << endl;
    } catch (const exception& e) {
        cerr << "Error restoring backup: " << e.what() << endl;
    }
}

//* function to pull last backup
void pullLastBackup() {
    try {
        vector<fs::directory_entry> backups;
        if (fs::exists(".backup") && fs::is_directory(".backup")) {
            for (const auto& entry : fs::directory_iterator(".backup")) {
                if (fs::is_directory(entry)) backups.push_back(entry);
            }
            sort(backups.begin(), backups.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
                return a.path().filename().string() > b.path().filename().string();
            });

            if (!backups.empty()) {
                restoreBackup(backups[0].path());
            } else {
                cout << "No backups found." << endl;
            }
        }
    } catch (const exception& e) {
        cerr << "Error pulling last backup: " << e.what() << endl;
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
    cout << "  backup meta            -> Show backup meta information\n";
    cout << "  backup help            -> Show available commands\n";
}

void showBackupMeta() {
    try {
        ifstream metaFile(".backup/__init__");
        if (!metaFile) {
            cout << "No backup metadata found. Is the backup system initialized?" << endl;
            return;
        }
        cout << "Backup Metadata:" << endl;
        string line;
        while (getline(metaFile, line)) {
            cout << "  " << line << endl;
        }
    } catch (const exception& e) {
        cerr << "Error reading backup metadata: " << e.what() << endl;
    }
}

//* main function
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: backup <command/help>\n";
        return 1;
    }

    string command = argv[1];

    if (command != "init" && command != "help") {
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
    } else if (command == "pull" && argc == 3 && string(argv[2]) == "--last") {
        pullLastBackup();
    } else if (command == "meta") {
        showBackupMeta();
    } else if (command == "help") {
        showHelp();
    } else {
        cout << "Invalid command. Use `backup help` for options.\n";
    }

    return 0;
}
