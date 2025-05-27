
//* TESTSCRIPT FOR BACKUP SYSTEM
//* DELETE LATER

#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>

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

//* function to initialize backup system
void initBackup() {
    fs::create_directories(".backup");
    cout << "Backup system initialized in `.backup/` folder." << endl;
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

//* function to show help menu
void showHelp() {
    cout << "Backup Manager Commands:\n";
    cout << "  backup init            -> Initialize backup system\n";
    cout << "  backup do              -> Create a new backup\n";
    cout << "  backup auto --min X    -> Auto backup every X minutes\n";
    cout << "  backup remove --all    -> Delete all backups\n";
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
    } else {
        cout << "Invalid command. Use `backup help` for options.\n";
    }

    return 0;
}
