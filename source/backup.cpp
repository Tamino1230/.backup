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

const string BACKUP_VERSION = "1.1.2";

// Function prototype for logAction
void logAction(const std::string& entry);

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

//* function to show version
void showVersion() {
    cout << ".backup Version: " << BACKUP_VERSION << endl;
}

//* function to get log directory in %LOCALAPPDATA%/backup-setup/logs
string getLogDir() {
#ifdef _WIN32
    char* appdata = nullptr;
    size_t len = 0;
    _dupenv_s(&appdata, &len, "LOCALAPPDATA");
    string dir = appdata ? string(appdata) + "\\backup-setup\\logs" : "backup-setup/logs";
    if (appdata) free(appdata);
    return dir;
#else
    const char* appdata = getenv("XDG_DATA_HOME");
    string dir = appdata ? string(appdata) + "/backup-setup/logs" : string(getenv("HOME")) + "/.local/share/backup-setup/logs";
    return dir;
#endif
}

void printLogs() {
    try {
        string logDir = getLogDir();
        string logFile = logDir + "/.backup-logs";
        ifstream log(logFile);
        if (!log) {
            cout << "No logs found." << endl;
            return;
        }
        cout << "Backup Logs:" << endl;
        string line;
        while (getline(log, line)) {
            cout << "  " << line << endl;
        }
    } catch (const exception& e) {
        cerr << "Error reading logs: " << e.what() << endl;
    }
}

void copyLogsToCurrentDir() {
    try {
        string logDir = getLogDir();
        string logFile = logDir + "/.backup-logs";
        string destFile = "./.backup-logs";
        if (!fs::exists(logFile)) {
            cout << "No logs found to copy." << endl;
            return;
        }
        fs::copy_file(logFile, destFile, fs::copy_options::overwrite_existing);
        cout << "Logs copied to " << destFile << endl;
    } catch (const exception& e) {
        cerr << "Error copying logs: " << e.what() << endl;
    }
}



//* function to read .backupignore patterns
set<string> readBackupIgnore() {
    set<string> ignore;
    ifstream ignoreFile(".backupignore");
    string line;
    while (getline(ignoreFile, line)) {
        // Trim whitespace
        line.erase(line.begin(), find_if(line.begin(), line.end(), [](unsigned char ch) { return !isspace(ch); }));
        line.erase(find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !isspace(ch); }).base(), line.end());
        if (line.empty() || line[0] == '#') continue;
        // If the line is a directory, add all files in that directory
        if (fs::is_directory(line)) {
            for (const auto& entry : fs::directory_iterator(line)) {
                ignore.insert(entry.path().filename().string());
            }
        } else {
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
        logAction("Created backup directory: " + backupDir);

        // The backup is created inside the .backup directory, which is in the current working directory.
        // Files and folders from the current directory (except those in .backupignore and .backup itself) are copied.

        for (const auto& file : fs::directory_iterator(".")) {
            string filename = file.path().filename().string();
            if (filename == ".backup") continue;
            if (filename == ".backupignore") {
                // Always include .backupignore in backup
            } else if (ignore.count(filename)) {
                logAction("Ignored by .backupignore: " + filename);
                continue;
            }
            fs::copy(file.path(), backupDir + "/" + filename, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
            logAction("Saved file to backup: " + filename + " -> " + backupDir + "/" + filename);
        }
        cout << "Backup saved to: " << backupDir << endl;
        logAction("Backup completed: " + backupDir);
    } catch (const exception& e) {
        cerr << "Error creating backup: " << e.what() << endl;
        logAction(string("ERROR: ") + e.what());
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
        logAction("All backups removed from .backup/");
    } catch (const exception& e) {
        cerr << "Error removing backups: " << e.what() << endl;
        logAction(string("ERROR: ") + e.what());
    }
}

//* function to restore from a backup directory
void restoreBackup(const fs::path& backupDir) {
    try {
        for (const auto& file : fs::directory_iterator(backupDir)) {
            fs::copy(file.path(), "./" + file.path().filename().string(), fs::copy_options::overwrite_existing);
            logAction("Restored file: " + file.path().filename().string() + " from " + backupDir.string());
        }
        cout << "Restored from backup: " << backupDir.string() << endl;
        logAction("Restored from backup: " + backupDir.string());
    } catch (const exception& e) {
        cerr << "Error restoring backup: " << e.what() << endl;
        logAction(string("ERROR: ") + e.what());
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
    cout << ".backup Commands:\n";
    cout << "  backup init              -> Initialize backup system\n";
    cout << "  backup do                -> Create a new backup\n";
    cout << "  backup auto --min X      -> Auto backup every X minutes\n";
    cout << "  backup remove --all      -> Delete all backups\n";
    cout << "  backup pull --last       -> Restore from the last backup\n";
    cout << "  backup meta              -> Show backup meta information\n";
    cout << "  backup logs              -> Show backup logs\n";
    cout << "  backup logs --copy       -> Copy logs to current directory\n";
    cout << "  backup --version | --v   -> Show version\n";
    cout << "  backup help              -> Show available commands\n";
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

//* function to log to .backup-logs in logs/ folder in %LOCALAPPDATA%/backup-setup
void logAction(const string& entry) {
    // Log to appdata
    string logDir = getLogDir();
    fs::create_directories(logDir);
    string logFile = logDir + "/.backup-logs";
    ofstream log(logFile, ios::app);
    if (log) {
        time_t now = time(nullptr);
        tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        char timebuf[32];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &localTime);
        log << "[" << timebuf << "] " << entry << endl;
    }
}

//* function to parse and execute commands
void executeCommand(const string& cmd) {
    if (cmd == "backup --version") {
        showVersion();
        logAction("Ran: backup --version");
    } else if (cmd == "backup --v") {
        showVersion();
        logAction("Ran: backup --v");
    } else if (cmd == "backup logs") {
        printLogs();
        logAction("Ran: backup logs");
    } else if (cmd == "backup logs --copy") {
        copyLogsToCurrentDir();
        logAction("Ran: backup logs --copy");
    } else if (cmd == "backup init") {
        initBackup();
        logAction("Ran: backup init");
    } else if (cmd == "backup do") {
        if (isBackupInitialized()) {
            createBackup();
            logAction("Ran: backup do");
        } else {
            cerr << "Backup not initialized. Run `backup init` first." << endl;
            logAction("ERROR: Not initialized, attempted backup do");
        }
    } else if (cmd.find("backup auto --min ") == 0) {
        int minutes = stoi(cmd.substr(18));
        if (isBackupInitialized()) {
            thread([minutes]() { autoBackup(minutes); }).detach();
            cout << "Automatic backup set every " << minutes << " minutes." << endl;
            logAction("Ran: backup auto --min " + to_string(minutes));
        } else {
            cerr << "Backup not initialized. Run `backup init` first." << endl;
            logAction("ERROR: Not initialized, attempted backup auto");
        }
    } else if (cmd == "backup remove --all") {
        removeAllBackups();
        logAction("Ran: backup remove --all");
    } else if (cmd == "backup pull --last") {
        pullLastBackup();
        logAction("Ran: backup pull --last");
    } else if (cmd == "backup meta") {
        showBackupMeta();
        logAction("Ran: backup meta");
    } else if (cmd == "backup help") {
        showHelp();
        logAction("Ran: backup help");
    } else if (cmd.rfind("backup ", 0) == 0) {
        cerr << "Unknown or invalid arguments for command: '" << cmd.substr(7) << "'\nUse 'backup help' for available commands." << endl;
        logAction("ERROR: Invalid arguments for command: " + cmd);
    } else {
        cerr << "Unknown command. Use 'backup help' for available commands." << endl;
        logAction("ERROR: Unknown command: " + cmd);
    }
}

int main(int argc, char* argv[]) {
    try {
        // setWorkingDirToExe(); // Entfernt, damit .backup im aktuellen Ordner bleibt
        if (argc > 1) {
            string cmd = "backup";
            for (int i = 1; i < argc; ++i) {
                cmd += " ";
                cmd += argv[i];
            }
            executeCommand(cmd);
        } else {
            cout << "Usage: `backup (command/help)`\n";
            cout << "Type 'backup help' for available commands." << endl;
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
    return 0;
}
