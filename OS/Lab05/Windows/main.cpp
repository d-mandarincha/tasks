#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <cstring>

const int N = 26;

struct ProcessInfo {
    HANDLE hInputRead;
    HANDLE hInputWrite;
    HANDLE hOutputRead;
    HANDLE hOutputWrite;
    PROCESS_INFORMATION pi;
};

bool CreateProcessWithPipes(ProcessInfo& proc, const std::string& command) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&proc.hInputRead, &proc.hInputWrite, &sa, 0)) {
        return false;
    }

    if (!CreatePipe(&proc.hOutputRead, &proc.hOutputWrite, &sa, 0)) {
        return false;
    }

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);
    si.hStdInput = proc.hInputRead;
    si.hStdOutput = proc.hOutputWrite;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.dwFlags = STARTF_USESTDHANDLES;

    ZeroMemory(&proc.pi, sizeof(PROCESS_INFORMATION));

    char* cmdLine = new char[command.length() + 1];
    strcpy(cmdLine, command.c_str());

    BOOL success = CreateProcessA(
        NULL,
        cmdLine,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &proc.pi);

    delete[] cmdLine;

    if (!success) {
        return false;
    }

    CloseHandle(proc.hInputRead);
    CloseHandle(proc.hOutputWrite);

    return true;
}

void PipeToPipe(HANDLE hSource, HANDLE hDest) {
    char buffer[4096];
    DWORD bytesRead, bytesWritten;

    while (ReadFile(hSource, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        WriteFile(hDest, buffer, bytesRead, &bytesWritten, NULL);
    }
}

int main() {
    std::cout << "ENTER THE NUMBERS SEPARATED BY A SPACE: ";
    std::string inputStr;
    std::getline(std::cin, inputStr);

    ProcessInfo procM, procA, procP, procS;

    if (!CreateProcessWithPipes(procM, "process_m.exe")) {
        std::cerr << "Can't start process_m.exe" << std::endl;
        return 1;
    }

    if (!CreateProcessWithPipes(procA, "process_a.exe")) {
        std::cerr << "Can't start process_a.exe" << std::endl;
        return 1;
    }

    if (!CreateProcessWithPipes(procP, "process_p.exe")) {
        std::cerr << "Can't start process_p.exe" << std::endl;
        return 1;
    }

    if (!CreateProcessWithPipes(procS, "process_s.exe")) {
        std::cerr << "Can't start process_s.exe" << std::endl;
        return 1;
    }

    std::string dataToSend = inputStr + "\n";
    DWORD bytesWritten;

    if (!WriteFile(procM.hInputWrite, dataToSend.c_str(), dataToSend.length(), &bytesWritten, NULL)) {
        std::cerr << "Failed to write to process_m.exe. Error: " << GetLastError() << std::endl;
    }

    std::cout << "Sent " << bytesWritten << " bytes to process_m.exe" << std::endl;
    WriteFile(procM.hInputWrite, dataToSend.c_str(), dataToSend.length(), &bytesWritten, NULL);
    CloseHandle(procM.hInputWrite);

    std::cout << "Piping M->A" << std::endl;
    PipeToPipe(procM.hOutputRead, procA.hInputWrite);
    std::cout << "Finished M->A" << std::endl;
    CloseHandle(procM.hOutputRead);
    CloseHandle(procA.hInputWrite);

    std::cout << "Piping A->P" << std::endl;
    PipeToPipe(procA.hOutputRead, procP.hInputWrite);
    CloseHandle(procA.hOutputRead);
    std::cout << "Finished A->P" << std::endl;
    CloseHandle(procP.hInputWrite);

    std::cout << "Piping P->S" << std::endl;
    PipeToPipe(procP.hOutputRead, procS.hInputWrite);
    std::cout << "Finished P->S" << std::endl;
    CloseHandle(procP.hOutputRead);
    CloseHandle(procS.hInputWrite);

    char buffer[4096];
    DWORD bytesRead;
    std::string finalResult;

    while (ReadFile(procS.hOutputRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        finalResult.append(buffer, bytesRead);
    }

    std::cout << "Result: " << finalResult << std::endl;

    CloseHandle(procS.hOutputRead);

    WaitForSingleObject(procM.pi.hProcess, INFINITE);
    WaitForSingleObject(procA.pi.hProcess, INFINITE);
    WaitForSingleObject(procP.pi.hProcess, INFINITE);
    WaitForSingleObject(procS.pi.hProcess, INFINITE);

    CloseHandle(procM.pi.hProcess);
    CloseHandle(procM.pi.hThread);
    CloseHandle(procA.pi.hProcess);
    CloseHandle(procA.pi.hThread);
    CloseHandle(procP.pi.hProcess);
    CloseHandle(procP.pi.hThread);
    CloseHandle(procS.pi.hProcess);
    CloseHandle(procS.pi.hThread);

    return 0;
}