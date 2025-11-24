#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <locale.h>
#include <sstream>

class ProcessKiller {
public:

    static bool KillProcessById(DWORD processId) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
        if (hProcess == NULL) {
            std::wcerr << L"Can't open process with ID " << processId
                << L". Ошибка: " << GetLastError() << std::endl;
            return false;
        }

        if (!TerminateProcess(hProcess, 0)) {
            std::wcerr << L"Can't end process with ID " << processId
                << L". Ошибка: " << GetLastError() << std::endl;
            CloseHandle(hProcess);
            return false;
        }

        std::wcout << L"Process with ID " << processId << L" Successfuly ends." << std::endl;
        CloseHandle(hProcess);
        return true;
    }

    static bool KillProcessesByName(const std::wstring& processName) {
        bool killedAny = false;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (hSnapshot == INVALID_HANDLE_VALUE) {
            std::wcerr << L"Can't do process snapshot. Error "
                << GetLastError() << std::endl;
            return false;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                std::wstring currentProcessName = pe32.szExeFile;
                if (_wcsicmp(currentProcessName.c_str(), processName.c_str()) == 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProcess != NULL) {
                        if (TerminateProcess(hProcess, 0)) {
                            std::wcout << L"Process " << currentProcessName
                                << L" (ID: " << pe32.th32ProcessID
                                << L") end." << std::endl;
                            killedAny = true;
                        }
                        else {
                            std::wcerr << L"Can't end process " << currentProcessName
                                << L". Error " << GetLastError() << std::endl;
                        }
                        CloseHandle(hProcess);
                    }
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);

        if (!killedAny) {
            std::wcout << L"Process with name '" << processName << L"' didn't find." << std::endl;
        }

        return killedAny;
    }

    static std::vector<std::wstring> GetProcessesFromEnvironment() {
        std::vector<std::wstring> processes;

        wchar_t buffer[4096];
        DWORD length = GetEnvironmentVariableW(L"PROC_TO_KILL", buffer, 4096);

        if (length == 0) {
            return processes;
        }

        std::wstring envValue = buffer;

        //Разделение по запятой
        size_t pos = 0;
        while ((pos = envValue.find(L',')) != std::wstring::npos) {
            processes.push_back(envValue.substr(0, pos));
            envValue.erase(0, pos + 1);
        }

        if (!envValue.empty()) {
            processes.push_back(envValue);
        }

        return processes;
    }

    // Завершить процессы из переменной окружения
    static void KillProcessesFromEnvironment() {
        std::wcout << L"Checking environment variable PROC_TO_KILL..." << std::endl;

        auto processesToKill = GetProcessesFromEnvironment();

        if (processesToKill.empty()) {
            std::wcout << L"Variable PROC_TO_KILL not found or empty" << std::endl;
            return;
        }

        for (const auto& procName : processesToKill) {
            KillProcessesByName(procName);
        }
    }
};

// Главная функция
int wmain(int argc, wchar_t* argv[]) {
    _wsetlocale(LC_ALL, L"");

    if (argc == 1) {
        // Без аргументов - используем переменную окружения
        ProcessKiller::KillProcessesFromEnvironment();
        return 0;
    }

    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        std::wstring arg = argv[i];


        if (arg == L"--id") {
            if (i + 1 < argc) {
                try {
                    DWORD processId = std::stoul(argv[i + 1]);
                    ProcessKiller::KillProcessById(processId);
                    i++;
                }
                catch (const std::exception&) {
                    std::wcerr << L"Invalid process ID: " << argv[i + 1] << std::endl;
                    return 1;
                }
            }
            else {
                std::wcerr << L"Error: can't find ID after --id" << std::endl;
                return 1;
            }
        }
        else if (arg == L"--name") {
            if (i + 1 < argc) {
                ProcessKiller::KillProcessesByName(argv[i + 1]);
                i++;
            }
            else {
                std::wcerr << L"Error. can't find name after --name" << std::endl;
                return 1;
            }
        }
        else {
            std::wcerr << L"Unexpected argument: " << arg << std::endl;
            return 1;
        }
    }
    return 0;
}