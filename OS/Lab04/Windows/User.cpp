#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <locale.h>
#include <cstdlib>
#include <thread>
#include <chrono>

class TestProcessManager {
public:
    // Запуск тестового процесса
    static bool StartTestProcess(const std::wstring& processName) {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        
        std::wstring command = L"notepad.exe";
        if (processName == L"calc.exe") {
            command = L"calc.exe";
        }
        else if (processName == L"mspaint.exe") {
            command = L"mspaint.exe";
        }
        else if(processName == L"charmap.exe") {
            command = L"charmap.exe";
        }
        else{command = processName;}
        
        if (!CreateProcessW(
            NULL,                           // Имя модуля
            const_cast<wchar_t*>(command.c_str()), // Командная строка
            NULL,                           // Дескриптор процесса не наследуется
            NULL,                           // Дескриптор потока не наследуется
            FALSE,                          // Наследование дескрипторов
            0,                              // Флаги создания
            NULL,                           // Использовать окружение родителя
            NULL,                           // Использовать текущий каталог
            &si,                            // Указатель на STARTUPINFO
            &pi))                           // Указатель на PROCESS_INFORMATION
        {
            std::wcout << L"Couldn't start " << processName 
                      << L". Error: " << GetLastError() << std::endl;
            return false;
        }
        
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        
        // Даем процессу время запуститься
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::wcout << L"Started process : " << processName << std::endl;
        return true;
    }
    
    // Проверка существования процесса
    static bool IsProcessRunning(const std::wstring& processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        
        bool found = false;
        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                    found = true;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        
        CloseHandle(hSnapshot);
        return found;
    }
    
    // Получение ID процесса по имени
    static DWORD GetProcessIdByName(const std::wstring& processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return 0;
        }
        
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        
        DWORD pid = 0;
        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                    pid = pe32.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        
        CloseHandle(hSnapshot);
        return pid;
    }
    
    // Установка переменной окружения
    static bool SetEnvironmentVariableForProcess(const std::wstring& name, 
                                                 const std::wstring& value) {
        if (!SetEnvironmentVariableW(name.c_str(), value.c_str())) {
            std::wcout << L"Error setting environment variable: " 
                      << GetLastError() << std::endl;
            return false;
        }
        return true;
    }
    
    // Удаление переменной окружения
    static bool DeleteEnvironmentVariable(const std::wstring& name) {
        if (!SetEnvironmentVariableW(name.c_str(), NULL)) {
            std::wcout << L"Error of deliting environment variable: " 
                      << GetLastError() << std::endl;
            return false;
        }
        return true;
    }
    
    // Запуск Killer с аргументами
    static bool RunKiller(const std::wstring& arguments) {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        
        std::wstring command = L"Killer.exe " + arguments;
        
        std::wcout << L"Start: " << command << std::endl;
        
        if (!CreateProcessW(
            NULL,                           // Имя модуля
            const_cast<wchar_t*>(command.c_str()), // Командная строка
            NULL,                           // Дескриптор процесса не наследуется
            NULL,                           // Дескриптор потока не наследуется
            FALSE,                          // Наследование дескрипторов
            0,                              // Флаги создания
            NULL,                           // Использовать окружение родителя
            NULL,                           // Использовать текущий каталог
            &si,                            // Указатель на STARTUPINFO
            &pi))                           // Указатель на PROCESS_INFORMATION
        {
            std::wcout << L"Can't start Killer. Error: " 
                      << GetLastError() << std::endl;
            return false;
        }
        
        // Ждем завершения Killer
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        
        std::wcout << L"Killer ends woth code: " << exitCode << std::endl;
        return exitCode == 0;
    }
};

int wmain() {
    _wsetlocale(LC_ALL, L"");
        
    // 1. Тестирование завершения по ID
    std::wcout << L"\n1. Testing process by ID" << std::endl;
    
    if (TestProcessManager::StartTestProcess(L"notepad.exe")) {
        // Даем время запуститься
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        DWORD pid = TestProcessManager::GetProcessIdByName(L"notepad.exe");
        if (pid != 0) {
            std::wcout << L"Found notepad.exe with ID: " << pid << std::endl;
            
            TestProcessManager::RunKiller(L"--id " + std::to_wstring(pid));
            
            // Проверяем, что процесс завершен
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!TestProcessManager::IsProcessRunning(L"notepad.exe")) {
                std::wcout << L"Process ended" << std::endl;
            }
            else {
                std::wcout << L"Process didn't end" << std::endl;
            }
        }
    }
        
    // Запускаем несколько процессов
    for (int i = 0; i < 3; i++) {
        TestProcessManager::StartTestProcess(L"notepad.exe");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    if (TestProcessManager::IsProcessRunning(L"notepad.exe")) {
        
        TestProcessManager::RunKiller(L"--name notepad.exe");
        
        // Проверяем
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!TestProcessManager::IsProcessRunning(L"notepad.exe")) {
            std::wcout << L"All processes stopped notepad.exe" << std::endl;
        }
        else {
            std::wcout << L"Not All processes stopped" << std::endl;
        }
    }
    
    std::wcout << L"testing with PROC_TO_KILL" << std::endl;
    
    // Устанавливаем переменную окружения
    TestProcessManager::SetEnvironmentVariableForProcess(
        L"PROC_TO_KILL", 
        L"notepad.exe,charmap.exe,mspaint.exe"
    );
    
    // Запускаем тестовые процессы
    std::wcout << L"Запускаем тестовые процессы..." << std::endl;
    TestProcessManager::StartTestProcess(L"notepad.exe");
    TestProcessManager::StartTestProcess(L"mspaint.exe");

    for (int i = 0; i < 10; ++i) {
        TestProcessManager::StartTestProcess(L"charmap.exe");
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Проверяем, что процессы запущены
    bool notepadRunning = TestProcessManager::IsProcessRunning(L"notepad.exe");
    bool paintRunning = TestProcessManager::IsProcessRunning(L"mspaint.exe");
    
    std::wcout << L"Before Killer:" << std::endl;
    std::wcout << L"  notepad.exe: " << (notepadRunning ? L"Is going" : L"isn't going") << std::endl;
    std::wcout << L"  mspaint.exe: " << (paintRunning ? L"is going" : L"isn't going") << std::endl;
    std::wcout << L"charmap.exe: " << (TestProcessManager::IsProcessRunning(L"charmap.exe") ? L"Is going" : L"Isn't going") << std::endl;
    
    // Запускаем Killer без аргументов (должен использовать переменную окружения)
    TestProcessManager::RunKiller(L"");
    
    // Проверяем результаты
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    notepadRunning = TestProcessManager::IsProcessRunning(L"notepad.exe");
    paintRunning = TestProcessManager::IsProcessRunning(L"mspaint.exe");
    
    std::wcout << L"After Killer:" << std::endl;
    std::wcout << L"  notepad.exe: " << (notepadRunning ? L"Is going" : L"isn't going") << std::endl;
    std::wcout << L"  mspaint.exe: " << (paintRunning ? L"is going" : L"isn't going") << std::endl;

    if (!notepadRunning && !paintRunning) {
        std::wcout << L"All processes from PROC_TO_KILL ended" << std::endl;
    }
    else {
        std::wcout << L"Not all processes ended" << std::endl;
    }
    
    // 4. Тестирование пустой переменной окружения
    std::wcout << L"\n Testing with empty variable" << std::endl;
    
    // Устанавливаем пустую переменную
    TestProcessManager::SetEnvironmentVariableForProcess(L"PROC_TO_KILL", L"");
    
    // Запускаем Killer
    TestProcessManager::RunKiller(L"");
    
    // 5. Удаляем переменную окружения
    std::wcout << L"\n5. Deliting variable PROC_TO_KILL" << std::endl;
    if (TestProcessManager::DeleteEnvironmentVariable(L"PROC_TO_KILL")) {
        std::wcout << L"Variable deleted" << std::endl;
    }
    else {
        std::wcout << L"Error od deliting variable" << std::endl;
    }
    
    // Завершаем все тестовые процессы на случай, если что-то осталось
    system("taskkill /f /im notepad.exe >nul 2>&1");
    system("taskkill /f /im mspaint.exe >nul 2>&1");
    
    return 0;
}