#pragma once
#include "queue/queue.hpp"
#include <condition_variable>
#include <mutex>
#include <queue>

namespace dispatcher::queue {

class UnboundedQueue : public IQueue {
public:
    UnboundedQueue();

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    ~UnboundedQueue() override;

private:
    std::queue<std::function<void()>> queue_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    bool shutdown_;
};

}  // namespace dispatcher::queue