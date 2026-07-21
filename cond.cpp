#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <print>
// #include <random>
#include <vector>

class process {
    
    
private:
    std::mutex mt;
    std::queue<int> q;
    std::condition_variable cond_var;
    int N = 1e5;

public:

    template<typename... Args>
    void write(Args... args) {
        std::lock_guard guard(mt);
        (q.push(args), ...);
    }

    void writer() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            int T = std::rand() % 1000;
            for (int i = 0; i < T; ++i) {
                write(N--);
            }
            cond_var.notify_all();
        }
    }

    void reader() {
        while (true) {
            std::unique_lock lock_(mt);
            cond_var.wait(lock_, [&](){return !q.empty();});
            int top_ = q.front();
            q.pop();
            std::print("popped: {} BY {}\n", top_, std::this_thread::get_id());
            lock_.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(std::rand() % 1000));
        }
    }

};

int main() {
    process P;
    const int M = 5;
    std::vector<std::thread> readers;
    for (int i = 0; i < M; ++i) {
        readers.emplace_back(&process::reader, &P);
    }

    std::thread writer(&process::writer, &P);
    writer.join();

    for (int i = 0; i < M; ++i) readers[i].join();
    
    return 0;
}