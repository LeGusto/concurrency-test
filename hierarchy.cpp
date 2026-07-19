#include <mutex>
#include <thread>
#include <format>
#include <concepts>
#include <stdexcept>

class hierarchical_mutex {
    private:
        static thread_local inline int curr_locked = std::numeric_limits<int>::max();
        std::mutex internal_mutex;
        int mutex_order = std::numeric_limits<int>::max();
        int prev_order = std::numeric_limits<int>::max();
        bool is_locked = false;     
    public:

        hierarchical_mutex() : mutex_order(std::numeric_limits<int>::max()) {};
        hierarchical_mutex(int order) : mutex_order(order) {};
        void lock() {
            if (curr_locked <= mutex_order) {
                throw std::logic_error(std::format("mutex hierarchy order violated: locking {}, but {} is locked", mutex_order, curr_locked));
            }
            internal_mutex.lock();
            prev_order = curr_locked;
            curr_locked = mutex_order;
            is_locked = true;
        }

        bool unlock() {
            if (curr_locked <= mutex_order) {
                throw std::logic_error(std::format("mutex hierarchy order violated: unlocking {}, but {} is locked", mutex_order, curr_locked));
            }
            curr_locked = prev_order;
            internal_mutex.unlock();
            return true;
        }
};

template<typename M>
concept Lockable = requires(M m) {
    m.lock();
    m.unlock();
};

template<Lockable M>
class worker{
private:
    M m1;
    M m2;
public:
    void do_op() {
        std::lock_guard guard(m1);
        std::lock_guard guard2(m2);
    }
    void do_op2() {
        std::lock_guard guard(m2);
        std::lock_guard guard2(m1);
    }

    worker(int v1, int v2): m1(v1), m2(v2) {};
    worker() = default;
};

worker<hierarchical_mutex> w(1, 2);
const int N = 1e4;
void work() {
    for (int i = 0; i < N; ++i) {
        w.do_op();
        w.do_op2();
    }
}

int main() {
    std::thread t1(work), t2(work);
    t1.join(); t2.join();
    return 0;
}