#include <iostream>
#include <vector>
#include <windows.h>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <mutex>

class MatrixMultiplier {
private:
    std::vector<std::vector<int>> A;
    std::vector<std::vector<int>> B;
    std::vector<std::vector<int>> C;
    int N;
    std::mutex mtx;

    struct ThreadData {
        MatrixMultiplier* instance;
        int iBlock;
        int jBlock;
        int blockSize;
    };

public:
    MatrixMultiplier(int size) : N(size) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 20);

        A.resize(N, std::vector<int>(N));
        B.resize(N, std::vector<int>(N));
        C.resize(N, std::vector<int>(N, 0));

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i][j] = dis(gen);
                B[i][j] = dis(gen);
            }
        }
    }

    // Статическая функция для потока Windows
    static DWORD WINAPI MultiplyBlockThread(LPVOID lpParam) {
        ThreadData* data = static_cast<ThreadData*>(lpParam);
        data->instance->multiplyBlock(data->iBlock, data->jBlock, data->blockSize);
        delete data;
        return 0;
    }

    void multiplyBlock(int iBlock, int jBlock, int blockSize) {
        int rowStart = iBlock * blockSize;
        int rowEnd = std::min(rowStart + blockSize, N);
        int colStart = jBlock * blockSize;
        int colEnd = std::min(colStart + blockSize, N);

        std::vector<std::vector<int>> localResult(rowEnd - rowStart,
            std::vector<int>(colEnd - colStart, 0));

        for (int kBlock = 0; kBlock < (N + blockSize - 1) / blockSize; kBlock++) {
            int kStart = kBlock * blockSize;
            int kEnd = std::min(kStart + blockSize, N);

            for (int i = rowStart; i < rowEnd; i++) {
                for (int j = colStart; j < colEnd; j++) {
                    int sum = 0;
                    for (int k = kStart; k < kEnd; k++) {
                        sum += A[i][k] * B[k][j];
                    }
                    localResult[i - rowStart][j - colStart] += sum;
                }
            }
        }

        std::lock_guard<std::mutex> lock(mtx);
        for (int i = rowStart; i < rowEnd; i++) {
            for (int j = colStart; j < colEnd; j++) {
                C[i][j] += localResult[i - rowStart][j - colStart];
            }
        }
    }

    long long multiplyParallel(int blockSize) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                C[i][j] = 0;
            }
        }

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<HANDLE> threads;
        int numBlocks = (N + blockSize - 1) / blockSize;

        for (int iBlock = 0; iBlock < numBlocks; iBlock++) {
            for (int jBlock = 0; jBlock < numBlocks; jBlock++) {
                ThreadData* data = new ThreadData{this, iBlock, jBlock, blockSize};
                
                HANDLE hThread = CreateThread(
                    NULL,                   
                    0,                      
                    MultiplyBlockThread,    
                    data,                   
                    0,                      
                    NULL                    
                );
                
                if (hThread == NULL) {
                    std::cerr << "Error creating thread: " << GetLastError() << std::endl;
                    delete data;
                } else {
                    threads.push_back(hThread);
                }
            }
        }

        WaitForMultipleObjects(threads.size(), threads.data(), TRUE, INFINITE);

        for (HANDLE hThread : threads) {
            CloseHandle(hThread);
        }

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    bool verifyMultiplication(std::vector<std::vector<int>>& check) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (C[i][j] != check[i][j]) {
                    return false;
                }
            }
        }
        return true;
    }

    std::vector<std::vector<int>> computeStandard() {
        std::vector<std::vector<int>> standard(N, std::vector<int>(N, 0));
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                int sum = 0;
                for (int k = 0; k < N; k++) {
                    sum += A[i][k] * B[k][j];
                }
                standard[i][j] = sum;
            }
        }
        return standard;
    }
};

int main() {
    SetConsoleOutputCP(CP_UTF8);
    
    const int N = 80;
    MatrixMultiplier multiplier(N);
    std::vector<std::vector<int>> standard = multiplier.computeStandard();
    
    std::cout << "\n=== PERFORMANCE COMPARISON ===\n";
    std::cout << "Matrix size: " << N << "x" << N << "\n";
    std::cout << "\n2. Parallel algorithm with different block sizes (Windows Threads):\n";
    std::cout << std::setw(15) << "Block size"
              << std::setw(20) << "Number of blocks"
              << std::setw(20) << "Number of threads"
              << std::setw(20) << "Time (microsec)"
              << std::setw(20) << "Is Valid"
              << std::endl;
              
    for (int k : {1, 2, 4, 5, 8, 10, 20, 40, 80}) {
        int numBlocks = ((N + k - 1) / k) * ((N + k - 1) / k);
        long long parTime = multiplier.multiplyParallel(k);
        
        bool isValid = multiplier.verifyMultiplication(standard);
        
        std::cout << std::setw(15) << k << "x" << k
                  << std::setw(20) << numBlocks
                  << std::setw(20) << numBlocks
                  << std::setw(20) << parTime
                  << std::setw(20) << (isValid ? " [OK]" : " [ERROR]")
                  << std::endl;
    }    
    return 0;
}