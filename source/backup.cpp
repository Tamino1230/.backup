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
#include <ctime>
#include <set>
#ifdef _WIN32
#include <windows.h>
#endif

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

//* function to read .backupignore patterns
set<string> readBackupIgnore() {
    set<string> ignore;
    ifstream ignoreFile(".backupignore");
    string line;
    while (getline(ignoreFile, line)) {
        // Remove whitespace
        line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (!line.empty() && line[0] != '#') {
            ignore.insert(line);
        }
    }
    return ignore;
}

//* function to create a backup safely (with .backupignore support)
void createBackup() {
    try {
        set<string> ignore = readBackupIgnore();
        string backupDir = ".backup/Backup_" + getTimestamp();
        fs::create_directories(backupDir);

        for (const auto& file : fs::directory_iterator(".")) {
            string filename = file.path().filename().string();
            if (filename == ".backup") continue;
            if (filename == ".backupignore") {
                // Always include .backupignore in backup
            } else if (ignore.count(filename)) {
                continue;
            }
            fs::copy(file.path(), backupDir + "/" + filename, fs::copy_options::overwrite_existing);
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

//* function to get the path to the running executable
string getExecutableDir() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    string exePath(path);
    size_t pos = exePath.find_last_of("/\\");
    return (pos != string::npos) ? exePath.substr(0, pos) : "";
#else
    char result[1024];
    ssize_t count = readlink("/proc/self/exe", result, 1024);
    string exePath(count != -1 ? string(result, count) : "");
    size_t pos = exePath.find_last_of("/");
    return (pos != string::npos) ? exePath.substr(0, pos) : "";
#endif
}

//* function to set the working directory to the executable's directory
void setWorkingDirToExe() {
    try {
        string exeDir = getExecutableDir();
        if (!exeDir.empty()) {
            fs::current_path(exeDir);
        }
    } catch (const exception& e) {
        cerr << "Error setting working directory: " << e.what() << endl;
    }
}

//* function to parse and execute commands
void executeCommand(const string& cmd) {
    if (cmd == "backup init") {
        initBackup();
    } else if (cmd == "backup do") {
        if (isBackupInitialized()) {
            createBackup();
        } else {
            cerr << "Backup not initialized. Run `backup init` first." << endl;
        }
    } else if (cmd.find("backup auto --min ") == 0) {
        int minutes = stoi(cmd.substr(18));
        if (isBackupInitialized()) {
            thread([minutes]() { autoBackup(minutes); }).detach();
            cout << "Automatic backup set every " << minutes << " minutes." << endl;
        } else {
            cerr << "Backup not initialized. Run `backup init` first." << endl;
        }
    } else if (cmd == "backup remove --all") {
        removeAllBackups();
    } else if (cmd == "backup pull --last") {
        pullLastBackup();
    } else if (cmd == "backup meta") {
        showBackupMeta();
    } else if (cmd == "backup help") {
        showHelp();
    } else {
        cerr << "Unknown command: " << cmd << endl;
        showHelp();
    }
}

int main(int argc, char* argv[]) {
    try {
        setWorkingDirToExe();
        if (argc > 1) {
            string cmd = argv[1];
            executeCommand(cmd);
        } else {
            cout << "Welcome to Backup Manager!" << endl;
            showHelp();
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
    return 0;
}
