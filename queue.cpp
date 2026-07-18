#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <string>
#include <print>
#include <map>
#include <functional>

constexpr int N = 1e5;

class tracker {
    private:
        std::mutex mt;
        std::map<int, int> mp;
    public:
        void incr(int i) {
            std::lock_guard guard(mt);
            ++mp[i];
        }

        int peek(int i) {
            std::lock_guard guard(mt);
            return mp[i];
        }
};

template<typename T>
class mutex_q {
    private:
        std::recursive_mutex mt;
        std::queue<int> q;
    public:
        void pop() {
            // std::lock_guard guard(mt); 
            q.pop();
        }

        int front() {
            // std::lock_guard guard(mt);
            return q.front();
        }

        void push(T item) {
            // std::lock_guard guard(mt);
            q.push(item);
        }

        int front_and_pop() {
            std::lock_guard guard(mt);
            T rt = q.front();
            q.pop();
            return rt;
        }

        template <typename F, typename... Args>
        requires std::invocable<F, Args...>
    decltype(auto) lock_mt(F&& f, Args&&... args) {
        std::lock_guard guard(mt);
        return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    }

        // std::recursive_mutex& get_mt() {return mt;}
};


std::queue<int> q;
mutex_q<int> m_q;
tracker cnt;

// non thread-safe
void process_base() {
    for (int i = 0; i < N; ++i) {
        q.push(i);
        int top = q.front();
        q.pop();
        // std::print("thread: {}, top: {}s\n", std::this_thread::get_id(), top);
        cnt.incr(top);
        int t = cnt.peek(top);
        if (t > 2) std::cout<<"data race buh\n";
    }
}

// though the individual methods re atomic, a sequence of actions is not atomic and prone to a logic race
void process_mutex() {
    for (int i = 0; i < N; ++i) {
        m_q.push(i);
        int top = m_q.front();
        m_q.pop();
        // std::print("thread: {}, top: {}s\n", std::this_thread::get_id(), top);
        cnt.incr(top);
        int t = cnt.peek(top);
        if (t > 2) std::cout<<"data race buh\n";
    }
}

void process_lock_free() {

}

// group of actions maintain single invariant
void process_mutex2() {
    for (int i = 0; i < N; ++i) {
        m_q.push(i);
        int top = m_q.front_and_pop();
        // std::print("thread: {}, top: {}s\n", std::this_thread::get_id(), top);
        cnt.incr(top);
        int t = cnt.peek(top);
        if (t > 2) std::cout<<"data race buh\n";
    }
}

void process_mutex3() {
    for (int i = 0; i < N; ++i) {
        m_q.push(i);
        int top = 0;
        m_q.lock_mt([&]{
            top = m_q.front();
            m_q.pop();});
        // std::print("thread: {}, top: {}s\n", std::this_thread::get_id(), top);
        cnt.incr(top);
        int t = cnt.peek(top);
        if (t > 2) std::cout<<"data race buh\n";
    }
}



// int main() {
//     std::thread t1(process_mutex3), t2(process_mutex3);
//     t1.join();
//     t2.join();

// }


// ==================================


class obj  {
    private:
        int a = 0;
        std::mutex mt_a;
        int b = 0;
        std::mutex mt_b;
    public:
        void inc_both() { // scoped lock acquires all locks in one ops
            std::scoped_lock guard(mt_a, mt_b);
            ++a;
            ++b;
        }

        void print() {
            std::print("a: {}, b: {}", a, b);
        }
    
};

class atomic_o {
    private:
        std::atomic<int> a = 0;
        std::atomic<int> b = 0;
    public:

    void inc_both() { 
        ++a;
        ++b;
    }

    void print() {
        std::print("a: {}, b: {}", a.load(), b.load());
    }
};

template<typename T>
void work(T& o) {
    for (int i = 0; i < N; ++i) o.inc_both();
}

obj o;
atomic_o a_o;

int main() {
    std::thread t1(work<atomic_o>, std::ref(a_o)), t2(work<atomic_o>, std::ref(a_o));
    t1.join(); t2.join();
    a_o.print();
}