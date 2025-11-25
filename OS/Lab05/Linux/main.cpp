#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>
#include <fcntl.h>

struct ProcessInfo {
    int fdInputRead;    
    int fdInputWrite;   
    int fdOutputRead;   
    int fdOutputWrite;  
    pid_t pid;
};

bool CreateProcessWithPipes(ProcessInfo& proc, const std::string& command) {
    int pipeIn[2];   // pipeIn[0] - чтение, pipeIn[1] - запись (в процесс)
    int pipeOut[2];  // pipeOut[0] - чтение (из процесса), pipeOut[1] - запись
    
    if (pipe(pipeIn) == -1) {
        std::cerr << "pipeIn failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (pipe(pipeOut) == -1) {
        std::cerr << "pipeOut failed: " << strerror(errno) << std::endl;
        close(pipeIn[0]);
        close(pipeIn[1]);
        return false;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "fork failed: " << strerror(errno) << std::endl;
        close(pipeIn[0]);
        close(pipeIn[1]);
        close(pipeOut[0]);
        close(pipeOut[1]);
        return false;
    }
    
    if (pid == 0) {  
        close(pipeIn[1]);   // Не будем писать в stdin
        close(pipeOut[0]);  // Не будем читать из stdout
        
        if (dup2(pipeIn[0], STDIN_FILENO) == -1) {
            std::cerr << "dup2 stdin failed: " << strerror(errno) << std::endl;
            exit(1);
        }
        
        if (dup2(pipeOut[1], STDOUT_FILENO) == -1) {
            std::cerr << "dup2 stdout failed: " << strerror(errno) << std::endl;
            exit(1);
        }
        
        close(pipeIn[0]);
        close(pipeOut[1]);
        
        execlp(command.c_str(), command.c_str(), (char*)NULL);
        
        std::cerr << "execlp failed for " << command << ": " << strerror(errno) << std::endl;
        exit(1);
    } else {  
        close(pipeIn[0]);   // Закрываем чтение из pipeIn (будет читать дочерний)
        close(pipeOut[1]);  // Закрываем запись в pipeOut (будет писать дочерний)
        
        proc.fdInputWrite = pipeIn[1];   
        proc.fdOutputRead = pipeOut[0];
        proc.pid = pid;
        
        proc.fdInputRead = -1;
        proc.fdOutputWrite = -1;
        
        return true;
    }
}

void PipeToPipe(int fdSource, int fdDest) {
    char buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(fdSource, buffer, sizeof(buffer))) > 0) {
        ssize_t bytesWritten = 0;
        while (bytesWritten < bytesRead) {
            ssize_t result = write(fdDest, buffer + bytesWritten, bytesRead - bytesWritten);
            if (result == -1) {
                if (errno == EINTR) continue;  // Сигнал прервал вызов
                break;
            }
            bytesWritten += result;
        }
    }
}

int main() {
    std::cout << "ENTER THE NUMBERS SEPARATED BY A SPACE: ";
    std::string inputStr;
    std::getline(std::cin, inputStr);

    ProcessInfo procM, procA, procP, procS;

    if (!CreateProcessWithPipes(procM, "./process_m")) {
        std::cerr << "Can't start process_m" << std::endl;
        return 1;
    }

    if (!CreateProcessWithPipes(procA, "./process_a")) {
        std::cerr << "Can't start process_a" << std::endl;
        return 1;
    }

    if (!CreateProcessWithPipes(procP, "./process_p")) {
        std::cerr << "Can't start process_p" << std::endl;
        return 1;
    }

    if (!CreateProcessWithPipes(procS, "./process_s")) {
        std::cerr << "Can't start process_s" << std::endl;
        return 1;
    }

    std::string dataToSend = inputStr + "\n";
    
    // Пишем в процесс M
    ssize_t bytesWritten = write(procM.fdInputWrite, dataToSend.c_str(), dataToSend.length());
    if (bytesWritten == -1) {
        std::cerr << "Failed to write to process_m. Error: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Sent " << bytesWritten << " bytes to process_m" << std::endl;
    }
    close(procM.fdInputWrite); 

    // Пайпинг M -> A
    std::cout << "Piping M->A" << std::endl;
    PipeToPipe(procM.fdOutputRead, procA.fdInputWrite);
    std::cout << "Finished M->A" << std::endl;
    close(procM.fdOutputRead);
    close(procA.fdInputWrite);

    // Пайпинг A -> P
    std::cout << "Piping A->P" << std::endl;
    PipeToPipe(procA.fdOutputRead, procP.fdInputWrite);
    close(procA.fdOutputRead);
    std::cout << "Finished A->P" << std::endl;
    close(procP.fdInputWrite);

    // Пайпинг P -> S
    std::cout << "Piping P->S" << std::endl;
    PipeToPipe(procP.fdOutputRead, procS.fdInputWrite);
    std::cout << "Finished P->S" << std::endl;
    close(procP.fdOutputRead);
    close(procS.fdInputWrite);

    // Чтение финального результата из S
    char buffer[4096];
    std::string finalResult;
    ssize_t bytesRead;
    
    while ((bytesRead = read(procS.fdOutputRead, buffer, sizeof(buffer))) > 0) {
        finalResult.append(buffer, bytesRead);
    }
    
    std::cout << "Result: " << finalResult << std::endl;
    close(procS.fdOutputRead);

    waitpid(procM.pid, nullptr, 0);
    waitpid(procA.pid, nullptr, 0);
    waitpid(procP.pid, nullptr, 0);
    waitpid(procS.pid, nullptr, 0);

    return 0;
}