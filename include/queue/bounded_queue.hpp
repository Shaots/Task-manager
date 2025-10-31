#pragma once
#include "queue/queue.hpp"
#include <condition_variable>
#include <mutex>
#include <queue>

namespace dispatcher::queue {

class BoundedQueue : public IQueue {
public:
    explicit BoundedQueue(int capacity);

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    ~BoundedQueue() override;

private:
    std::queue<std::function<void()>> queue_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    const size_t capacity_;
    bool shutdown_;
};

}  // namespace dispatcher::queue