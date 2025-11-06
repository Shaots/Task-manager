#include "queue/unbounded_queue.hpp"

#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>

namespace dispatcher::queue {

UnboundedQueue::UnboundedQueue() : shutdown_(false) {}

void UnboundedQueue::push(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_) {
        return;
    }

    queue_.push(std::move(task));
    not_empty_.notify_one();
}

std::optional<std::function<void()>> UnboundedQueue::try_pop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (queue_.empty() || shutdown_) {
        return std::nullopt;
    }

    auto task = std::move(queue_.front());
    queue_.pop();
    return task;
}

UnboundedQueue::~UnboundedQueue() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
    }
    not_empty_.notify_all();
}

}  // namespace dispatcher::queue