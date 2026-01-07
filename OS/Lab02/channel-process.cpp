#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <random>
#include <mutex>
#include <sstream>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <functional>

template<typename T>
class BufferedChannel {
private:
    std::queue<T> buffer;
    size_t capacity;
    std::mutex mtx;
    std::condition_variable not_full;
    std::condition_variable not_empty;
    bool closed = false;
    
public:
    BufferedChannel(size_t cap) : capacity(cap) {}
    
    bool send(T item) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [this]() { return buffer.size() < capacity || closed; });
        
        if (closed) return false;
        
        buffer.push(std::move(item));
        not_empty.notify_one();
        return true;
    }
    
    bool receive(T& item) {
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [this]() { return !buffer.empty() || closed; });
        
        if (buffer.empty() && closed) return false;
        
        item = std::move(buffer.front());
        buffer.pop();
        not_full.notify_one();
        return true;
    }
    
    void close() {
        std::lock_guard<std::mutex> lock(mtx);
        closed = true;
        not_empty.notify_all();
        not_full.notify_all();
    }
    
    bool is_closed() {
        std::lock_guard<std::mutex> lock(mtx);
        return closed;
    }
};

class MatrixMultiplier {
private:
    std::vector<std::vector<int>> A;
    std::vector<std::vector<int>> B;
    std::vector<std::vector<int>> C;
    int N;
    std::mutex mtx; // Мьютекс для защиты доступа к C
    
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

    // Структура для передачи задания через канал
    struct Task {
        int iBlock;
        int jBlock;
        int blockSize;
    };

    void worker(BufferedChannel<Task>* taskChannel, BufferedChannel<bool>* doneChannel, 
                std::atomic<int>* activeWorkers) {
        Task task;
        while (taskChannel->receive(task)) {
            multiplyBlock(task.iBlock, task.jBlock, task.blockSize);
        }        
        if (activeWorkers->fetch_sub(1) == 1) {
            // Последний воркер закрывает doneChannel
            doneChannel->close();
        }
    }

    void multiplyBlock(int iBlock, int jBlock, int blockSize) {
        int rowStart = iBlock * blockSize;
        int rowEnd   = std::min(rowStart + blockSize, N);
        int colStart = jBlock * blockSize;
        int colEnd   = std::min(colStart + blockSize, N);

        for (int kBlock = 0; kBlock < (N + blockSize - 1) / blockSize; kBlock++) {
            int kStart = kBlock * blockSize;
            int kEnd   = std::min(kStart + blockSize, N);

            for (int i = rowStart; i < rowEnd; i++) {
                for (int j = colStart; j < colEnd; j++) {
                    int sum = 0;
                    for (int k = kStart; k < kEnd; k++) {
                        sum += A[i][k] * B[k][j];
                    }
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        C[i][j] += sum;
                    }
                }
            }
        }
    }

    long long multiplyParallel(int blockSize, int numThreads = std::thread::hardware_concurrency()) {
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                C[i][j] = 0;

        auto start = std::chrono::high_resolution_clock::now();

        // Создаем каналы
        BufferedChannel<Task> taskChannel(100); // Буферизированный канал задач
        BufferedChannel<bool> doneChannel(1);   // Канал для сигнала завершения
        
        std::atomic<int> activeWorkers(numThreads);
        
        std::vector<std::thread> workers;
        for (int i = 0; i < numThreads; i++) {
            workers.emplace_back(&MatrixMultiplier::worker, this, 
                                &taskChannel, &doneChannel, &activeWorkers);
        }

        int numBlocks = (N + blockSize - 1) / blockSize;
        int totalTasks = 0;
        
        for (int iBlock = 0; iBlock < numBlocks; iBlock++) {
            for (int jBlock = 0; jBlock < numBlocks; jBlock++) {
                Task task{iBlock, jBlock, blockSize};
                taskChannel.send(task);
                totalTasks++;
            }
        }
        taskChannel.close();

        bool dummy;
        while (doneChannel.receive(dummy)) {
            // Ждем сигнала от последнего воркера
        }

        // Дожидаемся завершения всех потоков
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
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
    const int N = 80;
    const int numThreads = std::thread::hardware_concurrency();
    MatrixMultiplier multiplier(N);
    std::vector<std::vector<int>> standard = multiplier.computeStandard();

    std::cout << "\n=== PERFORMANCE COMPARISON ===\n";
    std::cout << "Using " << numThreads << " worker threads\n";
    std::cout << "\n2. Parallel algorithm with different block sizes:\n";
    std::cout << std::setw(15) << "Block size"
              << std::setw(20) << "Number of blocks"
              << std::setw(20) << "Number of threads"
              << std::setw(20) << "Time (microsec)"
              << std::setw(20) << "Is Valid"
              << std::endl;
              
    for (int k : {1, 2, 4, 5, 8, 10, 20, 40, 80}) {
        int numBlocks = ((N + k - 1) / k) * ((N + k - 1) / k);
        long long parTime = multiplier.multiplyParallel(k, numThreads);
        
        bool isValid = multiplier.verifyMultiplication(standard);
        
        std::cout << std::setw(15) << k << "x" << k
                  << std::setw(20) << numBlocks
                  << std::setw(20) << numThreads
                  << std::setw(20) << parTime
                  << std::setw(20) << (isValid ? " [OK]" : " [ERROR]")
                  << std::endl;
    }
    
    return 0;
}