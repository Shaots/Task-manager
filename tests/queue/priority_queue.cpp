#include <gtest/gtest.h>

#include "queue/priority_queue.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

using namespace dispatcher::queue;

using namespace dispatcher;
class PriorityQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_ = {
            {TaskPriority::High, {true, 100}},   // Ограниченная очередь
            {TaskPriority::Normal, {false, {}}}  // Неограниченная очередь
        };
    }

    std::unordered_map<TaskPriority, QueueOptions> config_;
};

TEST_F(PriorityQueueTest, invalidConstructor) {
    std::unordered_map<TaskPriority, QueueOptions> invalid_config = {
        {TaskPriority::High, {true, {}}}  // capacity отсутствует
    };
    EXPECT_THROW(PriorityQueue pq(invalid_config), std::invalid_argument);

    invalid_config = {{TaskPriority::High, {true, -1}}};
    EXPECT_THROW(PriorityQueue pq(invalid_config), std::invalid_argument);

    EXPECT_NO_THROW(PriorityQueue pq(config_));
}

TEST_F(PriorityQueueTest, order) {
    PriorityQueue pq(config_);
    std::vector<int> execution_order;
    std::mutex order_mutex;

    pq.push(TaskPriority::Normal, [&execution_order, &order_mutex]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(3);
    });
    pq.push(TaskPriority::High, [&execution_order, &order_mutex]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(1);
    });
    pq.push(TaskPriority::Normal, [&execution_order, &order_mutex]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(4);
    });
    pq.push(TaskPriority::High, [&execution_order, &order_mutex]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(2);
    });

    std::vector<std::function<void()>> tasks;
    for (int i = 0; i < 4; ++i) {
        auto task = pq.pop();
        ASSERT_TRUE(task.has_value());
        tasks.push_back(std::move(*task));
    }

    for (auto &task : tasks) {
        task();
    }

    std::lock_guard<std::mutex> lock(order_mutex);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
    EXPECT_EQ(execution_order[2], 3);
    EXPECT_EQ(execution_order[3], 4);
}

TEST_F(PriorityQueueTest, pushPopSingleThread) {
    PriorityQueue pq(config_);

    bool high_executed = false;
    bool normal_executed = false;

    pq.push(TaskPriority::Normal, [&normal_executed]() { normal_executed = true; });
    pq.push(TaskPriority::High, [&high_executed]() { high_executed = true; });

    auto task1 = pq.pop();
    ASSERT_TRUE(task1.has_value());
    (*task1)();
    EXPECT_TRUE(high_executed);
    EXPECT_FALSE(normal_executed);

    auto task2 = pq.pop();
    ASSERT_TRUE(task2.has_value());
    (*task2)();
    EXPECT_TRUE(normal_executed);
}

TEST_F(PriorityQueueTest, popEmptyQueue) {
    PriorityQueue pq(config_);
    std::atomic<bool> pop_completed{false};
    std::atomic<bool> push_called{false};

    std::thread consumer([&pq, &pop_completed]() {
        auto task = pq.pop();  // block
        ASSERT_TRUE(task.has_value());
        (*task)();
        pop_completed = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ASSERT_FALSE(pop_completed.load());

    // unblock
    std::thread producer([&pq, &push_called]() {
        pq.push(TaskPriority::High, []() {});
        push_called = true;
    });

    producer.join();
    consumer.join();

    EXPECT_TRUE(pop_completed.load());
    EXPECT_TRUE(push_called.load());
}

TEST_F(PriorityQueueTest, shutdown) {
    PriorityQueue pq(config_);
    std::atomic<bool> pop_unblocked{false};

    std::thread consumer([&pq, &pop_unblocked]() {
        auto task = pq.pop();            // block
        EXPECT_FALSE(task.has_value());  // After shutdown should be nullopt
        pop_unblocked = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_FALSE(pop_unblocked.load());

    pq.shutdown();

    consumer.join();
    EXPECT_TRUE(pop_unblocked.load());
}

TEST_F(PriorityQueueTest, pushAndShutdown) {
    PriorityQueue pq(config_);

    std::atomic<int> executed_count{0};
    pq.push(TaskPriority::High, [&executed_count]() { executed_count++; });
    pq.push(TaskPriority::Normal, [&executed_count]() { executed_count++; });
    pq.shutdown();

    int popped_count = 0;
    while (auto task = pq.pop()) {
        (*task)();
        popped_count++;
    }

    EXPECT_EQ(popped_count, 2);
    EXPECT_EQ(executed_count.load(), 2);

    EXPECT_FALSE(pq.pop().has_value());
}