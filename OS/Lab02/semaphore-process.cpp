#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>
#include <semaphore.h>
#include <pthread.h>
#include <sstream>

class MatrixMultiplier {
private:
    std::vector<std::vector<int>> A;
    std::vector<std::vector<int>> B;
    std::vector<std::vector<int>> C;
    int N;
    int blockSize;
    int numBlocks;
    sem_t task_semaphore;
    sem_t write_semaphore;
    std::vector<int> block_tasks;
    pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    // Структура для передачи данных в поток
    struct ThreadData {
        MatrixMultiplier* multiplier;
        int thread_id;
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

    ~MatrixMultiplier() {
        sem_destroy(&task_semaphore);
        sem_destroy(&write_semaphore);
        pthread_mutex_destroy(&task_mutex);
    }

    void multiplyBlock(int iBlock, int jBlock) {
        int rowStart = iBlock * blockSize;
        int rowEnd   = std::min(rowStart + blockSize, N);
        int colStart = jBlock * blockSize;
        int colEnd   = std::min(colStart + blockSize, N);

        std::vector<std::vector<int>> localResult(rowEnd - rowStart, 
                                                  std::vector<int>(colEnd - colStart, 0));

        for (int kBlock = 0; kBlock < (N + blockSize - 1) / blockSize; kBlock++) {
            int kStart = kBlock * blockSize;
            int kEnd   = std::min(kStart + blockSize, N);

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
        
        // Захватываем семафор для записи в общую матрицу
        sem_wait(&write_semaphore);
        for (int i = rowStart; i < rowEnd; i++) {
            for (int j = colStart; j < colEnd; j++) {
                C[i][j] += localResult[i - rowStart][j - colStart];
            }
        }
        sem_post(&write_semaphore);
    }

    static void* workerThread(void* arg) {
        ThreadData* data = static_cast<ThreadData*>(arg);
        MatrixMultiplier* multiplier = data->multiplier;
        
        while (true) {
            // Ожидаем доступную задачу
            sem_wait(&multiplier->task_semaphore);
            
            int task_index = -1;
            
            pthread_mutex_lock(&multiplier->task_mutex);
            if (!multiplier->block_tasks.empty()) {
                task_index = multiplier->block_tasks.back();
                multiplier->block_tasks.pop_back();
            }
            pthread_mutex_unlock(&multiplier->task_mutex);
            
            if (task_index == -1) {
                sem_post(&multiplier->task_semaphore); // Возвращаем семафор для других потоков
                break;
            }
            
            int iBlock = task_index / multiplier->numBlocks;
            int jBlock = task_index % multiplier->numBlocks;
            multiplier->multiplyBlock(iBlock, jBlock);
        }
        
        delete data;
        pthread_exit(NULL);
        return nullptr;
    }

    long long multiplyParallel(int bs) {
        blockSize = bs;
        numBlocks = (N + blockSize - 1) / blockSize;
        
        // Инициализируем семафоры
        sem_init(&task_semaphore, 0, 0);
        sem_init(&write_semaphore, 0, 1); //для записи
        
        block_tasks.clear();
        for (int iBlock = 0; iBlock < numBlocks; iBlock++) {
            for (int jBlock = 0; jBlock < numBlocks; jBlock++) {
                block_tasks.push_back(iBlock * numBlocks + jBlock);
            }
        }
        
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                C[i][j] = 0;
            }
        }

        auto start = std::chrono::high_resolution_clock::now();

        unsigned int num_threads = 4; // По умолчанию 4 потока
        #ifdef _SC_NPROCESSORS_ONLN
        num_threads = sysconf(_SC_NPROCESSORS_ONLN);
        #endif
        if (num_threads < 1) num_threads = 4;
        
        pthread_t* threads = new pthread_t[num_threads];
        
        for (size_t i = 0; i < block_tasks.size(); i++) {
            sem_post(&task_semaphore);
        }
        
        for (unsigned int i = 0; i < num_threads; i++) {
            ThreadData* data = new ThreadData{this, static_cast<int>(i)};
            pthread_create(&threads[i], NULL, &MatrixMultiplier::workerThread, data);
        }

        for (unsigned int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
        
        delete[] threads;

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
    
    unsigned int hardware_threads = 4;
    
    for (int k : {1, 2, 4, 5, 8, 10, 20, 40, 80}) {
        int numBlocks = ((N + k - 1) / k) * ((N + k - 1) / k);
        long long parTime = multiplier.multiplyParallel(k);
        
        bool isValid = multiplier.verifyMultiplication(standard);
        
        std::cout << std::setw(15) << k << "x" << k
                  << std::setw(20) << numBlocks
                  << std::setw(20) << hardware_threads
                  << std::setw(20) << parTime
                  << std::setw(20) << (isValid ? " [OK]" : " [ERROR]")
                  << std::endl;
    }
    
    return 0;
}