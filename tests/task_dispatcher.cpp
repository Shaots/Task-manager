#include "task_dispatcher.hpp"
#include <future>
#include <gtest/gtest.h>

using namespace dispatcher;
using namespace queue;
class TaskDispatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Базовая конфигурация для тестов
        default_config_ = {{TaskPriority::High, {true, 100}}, {TaskPriority::Normal, {false, {}}}};
    }

    std::unordered_map<TaskPriority, QueueOptions> default_config_;
};

TEST_F(TaskDispatcherTest, constructor) {
    EXPECT_THROW(TaskDispatcher dispatcher(0), std::invalid_argument);
    EXPECT_THROW(TaskDispatcher dispatcher(100, default_config_), std::invalid_argument);
    EXPECT_NO_THROW(TaskDispatcher dispatcher(4));
    EXPECT_NO_THROW(TaskDispatcher dispatcher(2, default_config_));
}

TEST_F(TaskDispatcherTest, push) {
    TaskDispatcher dispatcher(2);

    EXPECT_NO_THROW(dispatcher.schedule(TaskPriority::High, []() {}));
    EXPECT_NO_THROW(dispatcher.schedule(TaskPriority::Normal, []() {}));
    EXPECT_THROW(dispatcher.schedule(TaskPriority::High, nullptr), std::invalid_argument);
}

TEST_F(TaskDispatcherTest, simpleExec) {
    TaskDispatcher dispatcher(2);
    std::atomic<bool> task_executed{false};
    std::promise<void> task_completed;

    dispatcher.schedule(TaskPriority::Normal, [&task_executed, &task_completed]() {
        task_executed = true;
        task_completed.set_value();
    });
    task_completed.get_future().get();
    EXPECT_TRUE(task_executed.load());
}

TEST_F(TaskDispatcherTest, order) {
    std::unordered_map<TaskPriority, QueueOptions> config = {{TaskPriority::High, {true, 10}},
                                                             {TaskPriority::Normal, {true, 10}}};

    TaskDispatcher dispatcher(2, config);
    std::vector<int> execution_order;
    std::mutex order_mutex;
    std::atomic<int> tasks_completed{0};
    std::promise<void> all_tasks_done;

    dispatcher.schedule(TaskPriority::Normal, [&execution_order, &order_mutex, &tasks_completed, &all_tasks_done]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Имитируем работу
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(3);
        if (++tasks_completed == 4)
            all_tasks_done.set_value();
    });

    dispatcher.schedule(TaskPriority::High, [&execution_order, &order_mutex, &tasks_completed, &all_tasks_done]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(1);
        if (++tasks_completed == 4)
            all_tasks_done.set_value();
    });

    dispatcher.schedule(TaskPriority::Normal, [&execution_order, &order_mutex, &tasks_completed, &all_tasks_done]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(4);
        if (++tasks_completed == 4)
            all_tasks_done.set_value();
    });

    dispatcher.schedule(TaskPriority::High, [&execution_order, &order_mutex, &tasks_completed, &all_tasks_done]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(2);
        if (++tasks_completed == 4)
            all_tasks_done.set_value();
    });

    all_tasks_done.get_future().get();

    std::lock_guard<std::mutex> lock(order_mutex);
    if (execution_order.size() >= 2) {
        EXPECT_EQ(execution_order[0], 1);
        EXPECT_EQ(execution_order[1], 2);
    }
    EXPECT_EQ(tasks_completed.load(), 4);
}

TEST_F(TaskDispatcherTest, multithreadExec) {
    const int num_tasks = 50;
    const int num_threads = 4;
    TaskDispatcher dispatcher(num_threads);
    std::atomic<int> tasks_completed{0};
    std::promise<void> all_tasks_done;

    for (int i = 0; i < num_tasks; ++i) {
        dispatcher.schedule(TaskPriority::Normal, [&tasks_completed, &all_tasks_done, num_tasks]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (++tasks_completed == num_tasks) {
                all_tasks_done.set_value();
            }
        });
    }
    all_tasks_done.get_future().get();
    EXPECT_EQ(tasks_completed.load(), num_tasks);
}

TEST_F(TaskDispatcherTest, complexSchedule) {
    TaskDispatcher dispatcher(4);
    const int num_producers = 5;
    const int tasks_per_producer = 20;
    const int total_tasks = num_producers * tasks_per_producer;

    std::atomic<int> total_tasks_completed{0};
    std::promise<void> all_tasks_completed;
    std::atomic<int> tasks_remaining{total_tasks};

    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back(
            [&dispatcher, &total_tasks_completed, &tasks_remaining, &all_tasks_completed, tasks_per_producer]() {
                for (int j = 0; j < tasks_per_producer; ++j) {
                    dispatcher.schedule(TaskPriority::Normal,
                                        [&total_tasks_completed, &tasks_remaining, &all_tasks_completed]() {
                                            total_tasks_completed++;
                                            if (--tasks_remaining == 0) {
                                                all_tasks_completed.set_value();
                                            }
                                        });
                }
            });
    }

    for (auto &producer : producers) {
        producer.join();
    }
    all_tasks_completed.get_future().get();
    EXPECT_EQ(total_tasks_completed.load(), total_tasks);
}