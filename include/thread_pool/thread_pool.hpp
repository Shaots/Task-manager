#pragma once
#include "queue/priority_queue.hpp"
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
namespace dispatcher::thread_pool {

class ThreadPool {
public:
    explicit ThreadPool(std::shared_ptr<queue::PriorityQueue> queue, size_t num_threads);
    ~ThreadPool();

private:
    std::shared_ptr<queue::PriorityQueue> queue_;
    std::vector<std::jthread> workers_;
    std::atomic<bool> shutdown_{false};

    void worker_function();
};

}  // namespace dispatcher::thread_pool
