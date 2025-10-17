#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <atomic>
#include <windows.h>

class MatrixMultiplier {
private:
    std::vector<std::vector<int>> A;
    std::vector<std::vector<int>> B;
    std::vector<std::vector<int>> C;
    int N;
    std::atomic<int> block_counter;

    struct ThreadData {
        MatrixMultiplier* multiplier;
        int blockI;
        int blockJ;
        int blockSize;
    };

    static DWORD WINAPI MultiplyBlockThread(LPVOID lpParam) {
        ThreadData* data = static_cast<ThreadData*>(lpParam);
        data->multiplier->multiplyBlockImpl(data->blockI, data->blockJ, data->blockSize);
        delete data;
        return 0;
    }

    void multiplyBlockImpl(int blockI, int blockJ, int blockSize) {
        int startRow = blockI * blockSize;
        int endRow = std::min(startRow + blockSize, N);
        int startCol = blockJ * blockSize;
        int endCol = std::min(startCol + blockSize, N);
        
        int blocksPerDim = (N + blockSize - 1) / blockSize;
        
        std::vector<std::vector<int>> tempBlock(endRow - startRow, 
                                               std::vector<int>(endCol - startCol, 0));
        
        for (int blockK = 0; blockK < blocksPerDim; blockK++) {
            int kStart = blockK * blockSize;
            int kEnd = std::min(kStart + blockSize, N);
            
            for (int i = startRow; i < endRow; i++) {
                for (int j = startCol; j < endCol; j++) {
                    int sum = 0;
                    for (int k = kStart; k < kEnd; k++) {
                        sum += A[i][k] * B[k][j];
                    }
                    tempBlock[i - startRow][j - startCol] += sum;
                }
            }
        }
        
        for (int i = startRow; i < endRow; i++) {
            for (int j = startCol; j < endCol; j++) {
                C[i][j] = tempBlock[i - startRow][j - startCol];
            }
        }
        
        block_counter.fetch_add(1);
    }

    void multiplyBlockImplOld(int startRow, int endRow, int startCol, int endCol) {
        int block_id = block_counter.fetch_add(1) + 1;
        for (int i = startRow; i < endRow; i++) {
            for (int j = startCol; j < endCol; j++) {
                int sum = 0;
                for (int k = 0; k < N; k++) {
                    sum += A[i][k] * B[k][j];
                }
                C[i][j] = sum;
            }
        }
    }

public:
    MatrixMultiplier(int size) : N(size), block_counter(0) {
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

    long long multiplyParallel(int blockSize, bool showCalculations = true) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                C[i][j] = 0;
            }
        }
        
        block_counter = 0;
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<HANDLE> threads;
        int blocksPerDim = (N + blockSize - 1) / blockSize;
        int totalBlocks = blocksPerDim * blocksPerDim;
        
        for (int blockI = 0; blockI < blocksPerDim; blockI++) {
            for (int blockJ = 0; blockJ < blocksPerDim; blockJ++) {
                ThreadData* data = new ThreadData();
                data->multiplier = this;
                data->blockI = blockI;
                data->blockJ = blockJ;
                data->blockSize = blockSize;
                
                HANDLE thread = CreateThread(NULL, 0, MultiplyBlockThread, data, 0, NULL);
                if (thread != NULL) {
                    threads.push_back(thread);
                } else {
                    delete data;
                }
            }
        }
        
        const DWORD MAX_WAIT_OBJECTS = 64;
        size_t totalThreads = threads.size();
        size_t processed = 0;
        
        while (processed < totalThreads) {
            size_t remaining = totalThreads - processed;
            size_t batchSize = std::min<size_t>(remaining, MAX_WAIT_OBJECTS);
            
            DWORD result = WaitForMultipleObjects(
                static_cast<DWORD>(batchSize),
                threads.data() + processed,
                TRUE,
                INFINITE
            );
            
            if (result == WAIT_FAILED) {
                break;
            }
            
            for (size_t i = 0; i < batchSize; i++) {
                CloseHandle(threads[processed + i]);
            }
            processed += batchSize;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        
        if (showCalculations) {
            std::cout << "\n========================================\n";
            std::cout << "ALL BLOCKS PROCESSED\n";
            std::cout << "========================================\n";
        }
        
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
    const int N = 80;
    MatrixMultiplier multiplier(N);
    std::vector<std::vector<int>> standard = multiplier.computeStandard();
    
    std::cout << "\n=== PERFORMANCE COMPARISON ===\n";
    std::cout << "\n2. Parallel algorithm with different block sizes:\n";
    std::cout << std::setw(15) << "Block size"
        << std::setw(20) << "Number of blocks"
        << std::setw(20) << "Number of threads"
        << std::setw(20) << "Time (microsec)"
        << std::setw(20) << "Is Valid"
        << std::endl;
        
    for (int k : { 1, 2, 4, 5, 8, 10, 20, 40, 80}) {
        int blocksPerDim = (N + k - 1) / k;
        int numBlocks = blocksPerDim * blocksPerDim;
        
        long long parTimeSimple = multiplier.multiplyParallel(k, false);
        
        std::cout << std::setw(15) << k << "x" << k
            << std::setw(20) << numBlocks
            << std::setw(20) << numBlocks
            << std::setw(20) << parTimeSimple;
            
        if (multiplier.verifyMultiplication(standard)) {
            std::cout << std::setw(20) << " [OK]";
        }
        else {
            std::cout << std::setw(20) << " [ERROR]";
        }
        std::cout << std::endl;
    }
    return 0;
}