#include "queue/bounded_queue.hpp"

namespace dispatcher::queue {

BoundedQueue::BoundedQueue(int capacity) : capacity_(static_cast<size_t>(capacity)), shutdown_(false) {
    if (capacity <= 0) {
        throw std::invalid_argument("Capacity must be positive");
    }
}

void BoundedQueue::push(std::function<void()> task) {
    std::unique_lock<std::mutex> lock(mutex_);

    not_full_.wait(lock, [this]() { return queue_.size() < capacity_ || shutdown_; });

    if (shutdown_) {
        return;
    }

    queue_.push(std::move(task));
    lock.unlock();
    not_empty_.notify_one();
}

std::optional<std::function<void()>> BoundedQueue::try_pop() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (queue_.empty() || shutdown_) {
        return std::nullopt;
    }

    auto task = std::move(queue_.front());
    queue_.pop();
    lock.unlock();
    not_full_.notify_one();

    return task;
}

BoundedQueue::~BoundedQueue() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
    }
    not_empty_.notify_all();
    not_full_.notify_all();
}

}  // namespace dispatcher::queue