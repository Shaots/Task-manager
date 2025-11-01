#pragma once

#include <memory>

#include "queue/priority_queue.hpp"
#include "thread_pool/thread_pool.hpp"
#include "types.hpp"

namespace dispatcher {

class TaskDispatcher {
public:
    explicit TaskDispatcher(size_t thread_count,
                            std::unordered_map<TaskPriority, queue::QueueOptions> config = {
                                {TaskPriority::High, {true, 1000}},  // Ограниченная очередь на 1000 задач
                                {TaskPriority::Normal, {false, {}}}  // Неограниченная очередь
                            });

    void schedule(TaskPriority priority, std::function<void()> task);
    ~TaskDispatcher();

private:
    std::shared_ptr<queue::PriorityQueue> priority_queue_;
    std::unique_ptr<thread_pool::ThreadPool> thread_pool_;
    size_t thread_count_;
};

}  // namespace dispatcher