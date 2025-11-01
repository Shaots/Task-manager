#include "queue/priority_queue.hpp"

namespace dispatcher::queue {

PriorityQueue::PriorityQueue(const std::unordered_map<TaskPriority, QueueOptions> &config) {
    for (const auto &[priority, options] : config) {
        if (options.bounded) {
            if (!options.capacity.has_value()) {
                throw std::invalid_argument("Bounded queue must have capacity");
            }
            if (options.capacity.value() <= 0) {
                throw std::invalid_argument("Capacity must be positive");
            }
            queues_[priority] = std::make_unique<BoundedQueue>(options.capacity.value());
        } else {
            queues_[priority] = std::make_unique<UnboundedQueue>();
        }
    }
}

void PriorityQueue::push(TaskPriority priority, std::function<void()> task) {
    if (shutdown_.load()) {
        return;
    }

    auto it = queues_.find(priority);
    if (it == queues_.end()) {
        throw std::invalid_argument("Unknown task priority");
    }

    it->second->push(std::move(task));
    task_available_.notify_one();
}

std::optional<std::function<void()>> PriorityQueue::pop() {
    std::unique_lock<std::mutex> lock(pop_mutex_);

    while (!shutdown_.load()) {
        auto high_queue = queues_.find(TaskPriority::High);
        if (high_queue != queues_.end()) {
            if (auto task = high_queue->second->try_pop()) {
                if (task.has_value())
                    return task;
            }
        }

        auto normal_queue = queues_.find(TaskPriority::Normal);
        if (normal_queue != queues_.end()) {
            if (auto task = normal_queue->second->try_pop()) {
                return task;
            }
        }
        task_available_.wait(lock);
    }

    auto high_queue = queues_.find(TaskPriority::High);
    if (high_queue != queues_.end()) {
        if (auto task = high_queue->second->try_pop()) {
            return task;
        }
    }

    auto normal_queue = queues_.find(TaskPriority::Normal);
    if (normal_queue != queues_.end()) {
        if (auto task = normal_queue->second->try_pop()) {
            return task;
        }
    }

    return std::nullopt;
}

void PriorityQueue::shutdown() {
    shutdown_.store(true);
    task_available_.notify_all();  // Будим все ожидающие потоки
}

PriorityQueue::~PriorityQueue() { shutdown(); }

}  // namespace dispatcher::queue