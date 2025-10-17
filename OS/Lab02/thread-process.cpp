#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <random>
#include <mutex>
#include <sstream>
#include <atomic>

class MatrixMultiplier {
private:
    std::vector<std::vector<int>> A;
    std::vector<std::vector<int>> B;
    std::vector<std::vector<int>> C;
    int N;
    std::atomic<int> block_counter;
    std::mutex mtx;

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

    void multiplyBlock(int startRow, int endRow, int startCol, int endCol, int blockNum, int totalBlocks) {
        int blockHeight = endRow - startRow;
        int blockWidth = endCol - startCol;
        
        std::vector<std::vector<int>> tempBlock(blockHeight, std::vector<int>(blockWidth, 0));
        
        int kBlocks = (N + blockHeight - 1) / blockHeight;
        
        for (int kb = 0; kb < kBlocks; kb++) {
            int kStart = kb * blockHeight;
            int kEnd = std::min(kStart + blockHeight, N);
            
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
        
        std::lock_guard<std::mutex> lock(mtx);
        for (int i = startRow; i < endRow; i++) {
            for (int j = startCol; j < endCol; j++) {
                C[i][j] = tempBlock[i - startRow][j - startCol];
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
        
        std::vector<std::thread> threads;
        
        int blocksPerRow = (N + blockSize - 1) / blockSize;
        int blocksPerCol = (N + blockSize - 1) / blockSize;
        int totalBlocks = blocksPerRow * blocksPerCol;
        
        int blockNum = 0;
        for (int i = 0; i < N; i += blockSize) {
            for (int j = 0; j < N; j += blockSize) {
                int endRow = std::min(i + blockSize, N);
                int endCol = std::min(j + blockSize, N);
                
                threads.emplace_back(&MatrixMultiplier::multiplyBlock, this,
                    i, endRow, j, endCol, ++blockNum, totalBlocks);
            }
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        
        if (showCalculations) {
            std::cout << "\n========================================\n";
            std::cout << "ALL BLOCKS PROCESSED\n";
            std::cout << "========================================\n";
        }
        
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    void printMatrices() {
        std::cout << "\n=== Matrix A (" << N << "x" << N << ") ===\n";
        printMatrix(A);
        std::cout << "\n=== Matrix B (" << N << "x" << N << ") ===\n";
        printMatrix(B);
    }

    void printResultMatrix() {
        std::cout << "\n=== Result Matrix C = A * B (" << N << "x" << N << ") ===\n";
        printMatrix(computeStandard());
    }

    void printMatrix(const std::vector<std::vector<int>>& matrix) {
        for (const auto& row : matrix) {
            for (int val : row) {
                std::cout << std::setw(6) << val;
            }
            std::cout << std::endl;
        }
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
              
    for (int k : {1, 2, 4, 5, 8, 10, 20, 40, 80}) {
        int numBlocks = ((N + k - 1) / k) * ((N + k - 1) / k);
        long long parTime = multiplier.multiplyParallel(k, false);
        
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