#include "thread_pool/thread_pool.hpp"
#include <iostream>
namespace dispatcher::thread_pool {

ThreadPool::ThreadPool(std::shared_ptr<queue::PriorityQueue> queue, size_t num_threads) : queue_(std::move(queue)) {

    if (num_threads == 0) {
        throw std::invalid_argument("Number of threads must be positive");
    }
    if (num_threads > std::thread::hardware_concurrency()) {
        throw std::invalid_argument("Number of threads cannot be more than supported number threads");
    }

    if (!queue_) {
        throw std::invalid_argument("PriorityQueue cannot be null");
    }

    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_function, this);
    }
}

void ThreadPool::worker_function() {
    while (!shutdown_.load(std::memory_order_acquire)) {
        auto task = queue_->pop();

        if (task.has_value()) {
            try {
                (*task)();
            } catch (const std::exception &e) {
                std::println(std::cerr, "Exception in thread pool task: {}", e.what());
            } catch (...) {
                std::println(std::cerr, "Unknown exception in thread pool task");
            }
        } else {
            break;
        }
    }
}

ThreadPool::~ThreadPool() {
    shutdown_.store(true, std::memory_order_release);
    queue_->shutdown();
    for (auto &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

}  // namespace dispatcher::thread_pool