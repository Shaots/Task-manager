#include "task_dispatcher.hpp"
#include <stdexcept>

namespace dispatcher {

TaskDispatcher::TaskDispatcher(size_t thread_count, std::unordered_map<TaskPriority, queue::QueueOptions> config)
    : thread_count_(thread_count) {

    if (thread_count == 0) {
        throw std::invalid_argument("Thread count must be positive");
    }
    if (thread_count > std::thread::hardware_concurrency()) {
        throw std::invalid_argument("Number of threads cannot be more than supported number threads");
    }
    priority_queue_ = std::make_shared<queue::PriorityQueue>(config);
    thread_pool_ = std::make_unique<thread_pool::ThreadPool>(priority_queue_, thread_count);
}

void TaskDispatcher::schedule(TaskPriority priority, std::function<void()> task) {
    if (!task) {
        throw std::invalid_argument("Task cannot be null");
    }

    priority_queue_->push(priority, std::move(task));
}

TaskDispatcher::~TaskDispatcher() {}

}  // namespace dispatcher