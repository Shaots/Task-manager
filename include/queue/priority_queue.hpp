#pragma once
#include "queue/bounded_queue.hpp"
#include "queue/unbounded_queue.hpp"
#include "types.hpp"

#include <atomic>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>

namespace dispatcher::queue {

class PriorityQueue {
public:
    explicit PriorityQueue(const std::unordered_map<TaskPriority, QueueOptions> &config);

    void push(TaskPriority priority, std::function<void()> task);
    // block on pop until shutdown is called
    // after that return std::nullopt on empty queue
    std::optional<std::function<void()>> pop();

    void shutdown();

    ~PriorityQueue();

private:
    std::map<TaskPriority, std::unique_ptr<IQueue>> queues_;
    std::atomic<bool> shutdown_{false};
    std::mutex pop_mutex_;
    std::condition_variable task_available_;
};

}  // namespace dispatcher::queue