#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <signal.h>
#include <dirent.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <algorithm>

class Killer {
public:
    static bool killByPid(pid_t processId) {
        if (kill(processId, SIGTERM) != 0) {
            std::cerr << "Failed to kill process with PID " << processId 
                      << ". Error: " << strerror(errno) << std::endl;
            return false;
        }
        
        std::cout << "Process with PID " << processId << " terminated." << std::endl;
        return true;
    }
    
    static bool killByName(const std::string& processName) {
        DIR* dir = opendir("/proc");
        if (!dir) {
            std::cerr << "Failed to open /proc directory. Error: " 
                      << strerror(errno) << std::endl;
            return false;
        }
        
        struct dirent* entry;
        bool killedAny = false;
        
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                char* endptr;
                long pid = strtol(entry->d_name, &endptr, 10);
                
                if (*endptr == '\0') {
                    char cmdlinePath[256];
                    snprintf(cmdlinePath, sizeof(cmdlinePath), 
                            "/proc/%ld/cmdline", pid);
                    
                    std::ifstream file(cmdlinePath);
                    if (file) {
                        std::string cmdline;
                        if (std::getline(file, cmdline)) {
                            size_t pos = cmdline.find_last_of('/');
                            if (pos != std::string::npos) {
                                cmdline = cmdline.substr(pos + 1);
                            }
                            
                            cmdline.erase(std::remove(cmdline.begin(), cmdline.end(), '\0'), 
                                         cmdline.end());
                            
                            if (!cmdline.empty() && 
                                (cmdline == processName || 
                                 cmdline.find(processName) != std::string::npos)) {
                                
                                if (kill(static_cast<pid_t>(pid), SIGTERM) == 0) {
                                    std::cout << "Process " << cmdline 
                                              << " (PID: " << pid 
                                              << ") terminated." << std::endl;
                                    killedAny = true;
                                } else {
                                    std::cerr << "Failed to kill process " << cmdline 
                                              << " (PID: " << pid 
                                              << "). Error: " << strerror(errno) << std::endl;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        closedir(dir);
        
        if (!killedAny) {
            std::cout << "No processes found with name: " << processName << std::endl;
        }
        
        return killedAny;
    }
    
    static std::vector<std::string> getProcessesFromEnv() {
        std::vector<std::string> processes;
        
        const char* envValue = getenv("PROC_TO_KILL");
        if (!envValue || strlen(envValue) == 0) {
            return processes;
        }
        
        std::string envStr = envValue;
        std::istringstream iss(envStr);
        std::string token;
        
        while (std::getline(iss, token, ',')) {
            token.erase(0, token.find_first_not_of(" \t\n\r"));
            token.erase(token.find_last_not_of(" \t\n\r") + 1);
            
            if (!token.empty()) {
                processes.push_back(token);
            }
        }
        
        return processes;
    }
    
    static void killFromEnvironment() {
        std::cout << "Reading environment variable PROC_TO_KILL..." << std::endl;
        
        auto processes = getProcessesFromEnv();
        
        if (processes.empty()) {
            std::cout << "PROC_TO_KILL is not set or is empty." << std::endl;
            std::cout << "Usage examples:" << std::endl;
            std::cout << "  export PROC_TO_KILL=\"firefox,chrome\"" << std::endl;
            std::cout << "  ./Killer" << std::endl;
            std::cout << "Or use command line arguments:" << std::endl;
            std::cout << "  ./Killer --name firefox" << std::endl;
            std::cout << "  ./Killer --id 1234" << std::endl;
            return;
        }
        
        std::cout << "Processes to kill: ";
        for (size_t i = 0; i < processes.size(); ++i) {
            std::cout << processes[i];
            if (i != processes.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
        
        for (const auto& procName : processes) {
            killByName(procName);
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc == 1) {
        Killer::killFromEnvironment();
        return 0;
    }
    
    std::string firstArg = argv[1];
    
    if (firstArg == "--help" || firstArg == "-h") {
        Killer::showUsage();
        return 0;
    }
    
    if (firstArg == "--id") {
        if (argc < 3) {
            std::cerr << "Error: PID is required after --id" << std::endl;
            Killer::showUsage();
            return 1;
        }
        
        try {
            pid_t pid = static_cast<pid_t>(std::stoi(argv[2]));
            if (!Killer::killByPid(pid)) {
                return 1;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: Invalid PID format: " << argv[2] << std::endl;
            return 1;
        }
    }
    else if (firstArg == "--name") {
        if (argc < 3) {
            std::cerr << "Error: Process name is required after --name" << std::endl;
            Killer::showUsage();
            return 1;
        }
        
        Killer::killByName(argv[2]);
    }
    else {
        std::cerr << "Error: Unknown argument: " << firstArg << std::endl;
        Killer::showUsage();
        return 1;
    }
    
    return 0;
}