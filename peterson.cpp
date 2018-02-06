#include <atomic>
#include <future>
#include <iostream>

class Lock {
 public:
    Lock() {
        flags_[0].store(false, std::memory_order::memory_order_release);
        flags_[1].store(false, std::memory_order::memory_order_release);
    }

    void acquire(int pid) {
        flags_[pid].store(true, std::memory_order::memory_order_release);
        after_you_.store(pid, std::memory_order::memory_order_seq_cst);
        for (; flags_[1 - pid].load(std::memory_order::memory_order_acquire) &&
               after_you_.load(std::memory_order::memory_order_acquire) == pid;)
            ;
    }

    void release(int pid) {
        flags_[pid].store(false, std::memory_order::memory_order_release);
    }

 private:
    std::atomic_bool flags_[2];
    std::atomic_int after_you_;
};

int cnt = 0;
Lock cnt_lock;

void run(int pid) {
    int const upper_bound = 10000000;
    static_assert(upper_bound < std::numeric_limits<int>::max() / 2, "");

    for (int i = 0; i < upper_bound; ++i) {
        cnt_lock.acquire(pid);
        ++cnt;
        cnt_lock.release(pid);
    }
}

int main() {
    auto f0 = std::async(std::launch::async, run, 0);
    auto f1 = std::async(std::launch::async, run, 1);
    f0.wait();
    f1.wait();
    std::cout << cnt << std::endl;
}