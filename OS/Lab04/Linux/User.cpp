#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <algorithm>

class TestProcessManager {
public:
    static bool startProcess(const std::string& processName) {
        pid_t pid = fork();
        
        if (pid == 0) {
            std::string command = processName;
            
            if (processName == "notepad") {
                command = "gedit";
            }
            else if (processName == "calc") {
                command = "gnome-calculator";
            }
            else if (processName == "mspaint") {
                command = "gimp";
            }
            else if (processName == "charmap") {
                command = "gucharmap";
            }
            
            execlp(command.c_str(), command.c_str(), NULL);
            exit(1);
        }
        else if (pid > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            return true;
        }
        
        return false;
    }
    
    static bool isProcessRunning(pid_t pid) {
        return kill(pid, 0) == 0;
    }
    
    static bool isProcessRunning(const std::string& processName) {
        DIR* dir = opendir("/proc");
        if (!dir) return false;
        
        struct dirent* entry;
        bool found = false;
        
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                char* endptr;
                long pid = strtol(entry->d_name, &endptr, 10);
                
                if (*endptr == '\0') {
                    char path[256];
                    snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid);
                    
                    std::ifstream file(path);
                    if (file) {
                        std::string cmdline;
                        std::getline(file, cmdline);
                        
                        size_t pos = cmdline.find_last_of('/');
                        if (pos != std::string::npos) {
                            cmdline = cmdline.substr(pos + 1);
                        }
                        
                        cmdline.erase(std::remove(cmdline.begin(), cmdline.end(), '\0'), cmdline.end());
                        
                        if (cmdline == processName) {
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
        
        closedir(dir);
        return found;
    }
    
    static pid_t getPidByName(const std::string& processName) {
        DIR* dir = opendir("/proc");
        if (!dir) return 0;
        
        struct dirent* entry;
        
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                char* endptr;
                long pid = strtol(entry->d_name, &endptr, 10);
                
                if (*endptr == '\0') {
                    char path[256];
                    snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid);
                    
                    std::ifstream file(path);
                    if (file) {
                        std::string cmdline;
                        std::getline(file, cmdline);
                        
                        size_t pos = cmdline.find_last_of('/');
                        if (pos != std::string::npos) {
                            cmdline = cmdline.substr(pos + 1);
                        }
                        
                        cmdline.erase(std::remove(cmdline.begin(), cmdline.end(), '\0'), cmdline.end());
                        
                        if (cmdline == processName) {
                            closedir(dir);
                            return static_cast<pid_t>(pid);
                        }
                    }
                }
            }
        }
        
        closedir(dir);
        return 0;
    }
    
    static int countProcessesByName(const std::string& processName) {
        DIR* dir = opendir("/proc");
        if (!dir) return 0;
        
        struct dirent* entry;
        int count = 0;
        
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                char* endptr;
                long pid = strtol(entry->d_name, &endptr, 10);
                
                if (*endptr == '\0') {
                    char path[256];
                    snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid);
                    
                    std::ifstream file(path);
                    if (file) {
                        std::string cmdline;
                        std::getline(file, cmdline);
                        
                        size_t pos = cmdline.find_last_of('/');
                        if (pos != std::string::npos) {
                            cmdline = cmdline.substr(pos + 1);
                        }
                        
                        cmdline.erase(std::remove(cmdline.begin(), cmdline.end(), '\0'), cmdline.end());
                        
                        if (cmdline == processName) {
                            count++;
                        }
                    }
                }
            }
        }
        
        closedir(dir);
        return count;
    }
    
    static bool setEnvironmentVariable(const std::string& name, const std::string& value) {
        return setenv(name.c_str(), value.c_str(), 1) == 0;
    }
    
    static bool deleteEnvironmentVariable(const std::string& name) {
        return unsetenv(name.c_str()) == 0;
    }
    
    static bool runKiller(const std::string& arguments = "") {
        std::string command = "./Killer " + arguments;
        
        std::cout << "Executing: " << command << std::endl;
        
        int result = system(command.c_str());
        int exitCode = WEXITSTATUS(result);
        
        std::cout << "Exit code: " << exitCode << std::endl;
        return exitCode == 0;
    }
    
    static void killAllTestProcesses() {
        system("pkill -f gedit 2>/dev/null");
        system("pkill -f gnome-calculator 2>/dev/null");
        system("pkill -f gimp 2>/dev/null");
        system("pkill -f gucharmap 2>/dev/null");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
};

int main() {
    TestProcessManager::killAllTestProcesses();
    
    bool allTestsPassed = true;
    
    std::cout << "\n=== TEST 1: Kill by ID ===" << std::endl;
    TestProcessManager::startProcess("notepad");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    pid_t geditPid = TestProcessManager::getPidByName("gedit");
    
    if (geditPid != 0) {
        std::cout << "Found PID: " << geditPid << std::endl;
        std::cout << "Before: " << (TestProcessManager::isProcessRunning(geditPid) ? "RUNNING" : "NOT RUNNING") << std::endl;
        
        TestProcessManager::runKiller("--id " + std::to_string(geditPid));
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        bool processStillRunning = TestProcessManager::isProcessRunning(geditPid);
        std::cout << "After: " << (processStillRunning ? "RUNNING" : "NOT RUNNING") << std::endl;
        
        bool testPassed = !processStillRunning;
        std::cout << "Result: " << (testPassed ? "PASS" : "FAIL") << std::endl;
        
        if (!testPassed) allTestsPassed = false;
    }
    
    std::cout << "\n=== TEST 2: Kill by name ===" << std::endl;
    for (int i = 0; i < 3; i++) {
        TestProcessManager::startProcess("notepad");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    int countBefore = TestProcessManager::countProcessesByName("gedit");
    std::cout << "Before: " << countBefore << " processes" << std::endl;
    
    TestProcessManager::runKiller("--name gedit");
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    int countAfter = TestProcessManager::countProcessesByName("gedit");
    std::cout << "After: " << countAfter << " processes" << std::endl;
    
    bool testPassed = (countAfter == 0);
    std::cout << "Result: " << (testPassed ? "PASS" : "FAIL") << std::endl;
    
    if (!testPassed) allTestsPassed = false;
    
    std::cout << "\n=== TEST 3: PROC_TO_KILL environment variable ===" << std::endl;
    TestProcessManager::setEnvironmentVariable("PROC_TO_KILL", "gedit,gimp");
    
    TestProcessManager::startProcess("notepad");
    TestProcessManager::startProcess("mspaint");
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    bool geditBefore = TestProcessManager::isProcessRunning("gedit");
    bool gimpBefore = TestProcessManager::isProcessRunning("gimp");
    
    std::cout << "Before: gedit=" << (geditBefore ? "RUNNING" : "NOT RUNNING");
    std::cout << ", gimp=" << (gimpBefore ? "RUNNING" : "NOT RUNNING") << std::endl;
    
    TestProcessManager::runKiller("");
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    bool geditAfter = TestProcessManager::isProcessRunning("gedit");
    bool gimpAfter = TestProcessManager::isProcessRunning("gimp");
    
    std::cout << "After: gedit=" << (geditAfter ? "RUNNING" : "NOT RUNNING");
    std::cout << ", gimp=" << (gimpAfter ? "RUNNING" : "NOT RUNNING") << std::endl;
    
    testPassed = !geditAfter && !gimpAfter;
    std::cout << "Result: " << (testPassed ? "PASS" : "FAIL") << std::endl;
    
    if (!testPassed) allTestsPassed = false;
    
    std::cout << "\n=== TEST 4: Empty PROC_TO_KILL ===" << std::endl;
    TestProcessManager::setEnvironmentVariable("PROC_TO_KILL", "");
    
    TestProcessManager::startProcess("notepad");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    TestProcessManager::runKiller("");
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    bool stillRunning = TestProcessManager::isProcessRunning("gedit");
    std::cout << "Process still running: " << (stillRunning ? "YES" : "NO") << std::endl;
    std::cout << "Result: " << (stillRunning ? "PASS" : "FAIL") << std::endl;
    
    if (!stillRunning) allTestsPassed = false;
    
    std::cout << "\n=== TEST 5: Delete environment variable ===" << std::endl;
    TestProcessManager::deleteEnvironmentVariable("PROC_TO_KILL");
    
    const char* envValue = getenv("PROC_TO_KILL");
    bool envExists = (envValue != nullptr);
    std::cout << "PROC_TO_KILL exists: " << (envExists ? "YES" : "NO") << std::endl;
    std::cout << "Result: " << (!envExists ? "PASS" : "FAIL") << std::endl;
    
    if (envExists) allTestsPassed = false;
    
    std::cout << "\n=== Cleanup ===" << std::endl;
    TestProcessManager::killAllTestProcesses();
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "All tests: " << (allTestsPassed ? "PASSED" : "FAILED") << std::endl;
    
    return allTestsPassed ? 0 : 1;
}