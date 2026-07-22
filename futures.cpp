#include <chrono>
#include <thread>
#include <future>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <print>
#include <vector>
#include <atomic>

int work1() {
    std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%500));
    return 0;
}

int work2() {
    std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%500));
    return 2;
}

class process {
private:
    std::mutex mt;
    std::condition_variable cv;
    std::queue<std::packaged_task<int()>> q;
    std::atomic<bool> done = false;
public:
    void processor() {
        while (!done) {
        std::packaged_task<int()> task;
        {
            std::unique_lock guard(mt);
            cv.wait(guard, [&](){return !q.empty() || done;});
            if (done) return;

            task = std::move(q.front());
            q.pop();
        }

        task();
        }
    }

    template<typename F>
    std::future<int> exec(F&& f) {
        std::packaged_task<int()> task(std::forward<F>(f));
        std::future<int> fut = task.get_future();
        std::lock_guard guard(mt);
        q.push(std::move(task));
        cv.notify_one();
        return fut;
    }

    void set_done() {std::lock_guard guard(mt); done = true; cv.notify_all();}
};

int main() {
    const int M = 5;
    process P;
    std::vector<std::thread> workers;
    for (int i = 0; i < M; ++i) workers.emplace_back(&process::processor, &P);

    const int N = 100;
    std::vector<std::future<int>> res;
    for (int i = 0; i < N; ++i) {
        res.emplace_back(P.exec(work1));
        res.emplace_back(P.exec(work2));
    }

    std::print("queued\n");

    for (int i = 0; i < N*2; ++i) {
       std::print("task {}: {}\n", i, res[i].get());
    }


    P.set_done();
    for (int i = 0; i < M; ++i) workers[i].join();    
    
    return 0;
}