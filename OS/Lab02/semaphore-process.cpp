#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <atomic>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

class MatrixMultiplier {
private:
    std::vector<std::vector<int>> A;
    std::vector<std::vector<int>> B;
    std::vector<std::vector<int>> C;
    int N; // Размер матрицы
    
    int blockSize;
    int blocksPerRow;
    int blocksPerCol;
    int totalBlocks;
    
    sem_t block_sem;
    pthread_mutex_t counter_mutex;
    int next_block;
    
    struct ThreadData {
        MatrixMultiplier* multiplier;
        int thread_id;
    };

public:
    MatrixMultiplier(int size) : N(size), next_block(0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 20);

        A.resize(N, std::vector<int>(N));
        B.resize(N, std::vector<int>(N));

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i][j] = dis(gen);
                B[i][j] = dis(gen);
            }
        }

        C.resize(N, std::vector<int>(N, 0));
        
        sem_init(&block_sem, 0, 0);
        pthread_mutex_init(&counter_mutex, NULL);
    }

    ~MatrixMultiplier() {
        sem_destroy(&block_sem);
        pthread_mutex_destroy(&counter_mutex);
    }

    int getA(int i, int j) const { return A[i][j]; }
    int getB(int i, int j) const { return B[i][j]; }
    int getC(int i, int j) const { return C[i][j]; }

    static void* threadFunction(void* arg) {
        ThreadData* data = static_cast<ThreadData*>(arg);
        data->multiplier->processBlocks(data->thread_id);
        return nullptr;
    }

    void processBlocks(int thread_id) {
        while (true) {
            pthread_mutex_lock(&counter_mutex);
            int blockNum = next_block++;
            pthread_mutex_unlock(&counter_mutex);
            
            if (blockNum >= totalBlocks) {
                break;
            }
            
            int blockRow = blockNum / blocksPerCol;
            int blockCol = blockNum % blocksPerCol;
            
            int startRow = blockRow * blockSize;
            int startCol = blockCol * blockSize;
            int endRow = std::min(startRow + blockSize, N);
            int endCol = std::min(startCol + blockSize, N);
            
            multiplyBlock(startRow, endRow, startCol, endCol);
        }
    }

    void multiplyBlock(int startRow, int endRow, int startCol, int endCol) {
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

    long long multiplyParallel(int bs, bool showCalculations = true) {
        blockSize = bs;
        blocksPerRow = (N + blockSize - 1) / blockSize;
        blocksPerCol = (N + blockSize - 1) / blockSize;
        totalBlocks = blocksPerRow * blocksPerCol;
        
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                C[i][j] = 0;
            }
        }  
        pthread_mutex_lock(&counter_mutex);
        next_block = 0;
        pthread_mutex_unlock(&counter_mutex);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
        num_threads = std::min(num_threads, totalBlocks);
        
        pthread_t threads[num_threads];
        ThreadData thread_data[num_threads];
        
        for (int i = 0; i < num_threads; i++) {
            thread_data[i].multiplier = this;
            thread_data[i].thread_id = i;
            pthread_create(&threads[i], NULL, threadFunction, &thread_data[i]);
        }
        
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
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

    std::vector<std::vector<int>> standart = multiplier.computeStandard();

    std::cout << "\n=== PERFORMANCE COMPARISON ===\n";

    std::cout << "\n2. Parallel algorithm with different block sizes:\n";
    std::cout << std::setw(15) << "Block size"
        << std::setw(20) << "Number of blocks"
        << std::setw(20) << "Number of threads"
        << std::setw(20) << "Time (microsec)"
        << std::setw(20) << "Is Valid"
        << std::endl;
    
    for (int k : { 1, 2, 4, 5, 8, 10, 20, 40, 80}) {
        int numBlocks = ((N + k - 1) / k) * ((N + k - 1) / k);
        long long parTimeSimple = multiplier.multiplyParallel(k, false);

        std::cout << std::setw(15) << k << "x" << k
            << std::setw(20) << numBlocks
            << std::setw(20) << sysconf(_SC_NPROCESSORS_ONLN)
            << std::setw(20) << parTimeSimple;

        if (multiplier.verifyMultiplication(standart)) {
            std::cout << std::setw(20) << " [OK]";
        }
        else {
            std::cout << std::setw(20) << " [ERROR]";
        }
       
        std::cout << std::endl;
    }
    return 0;
}