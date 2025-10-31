#include <gtest/gtest.h>

#include "queue/unbounded_queue.hpp"

using namespace dispatcher::queue;

TEST(UnboundedQueueTest, constructor) { EXPECT_NO_THROW(UnboundedQueue queue); }

TEST(UnboundedQueueTest, pushPop) {
    UnboundedQueue queue;
    EXPECT_FALSE(queue.try_pop().has_value());

    bool executed = false;
    queue.push([&executed]() { executed = true; });

    auto task = queue.try_pop();
    ASSERT_TRUE(task.has_value());
    (*task)();
    EXPECT_TRUE(executed);
}

TEST(UnboundedQueueTest, fifo) {
    UnboundedQueue queue;
    std::vector<int> execution_order;
    std::mutex order_mutex;

    // Push tasks in order
    for (int i = 0; i < 5; ++i) {
        queue.push([i, &execution_order, &order_mutex]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(i);
        });
    }

    for (int i = 0; i < 5; ++i) {
        auto task = queue.try_pop();
        ASSERT_TRUE(task.has_value());
        (*task)();
    }

    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(execution_order[i], i);
    }
}

TEST(UnboundedQueueTest, capacityUnlimit) {
    UnboundedQueue queue;
    const int large_number = 10000;

    for (int i = 0; i < large_number; ++i) {
        queue.push([]() {});
    }

    int popped_count = 0;
    while (auto task = queue.try_pop()) {
        popped_count++;
    }

    EXPECT_EQ(popped_count, large_number);
}

TEST(UnboundedQueueTest, ShutdownBehavior) {
    std::atomic<bool> push_completed{false};
    std::thread producer;

    {
        UnboundedQueue queue;
        producer = std::thread([&queue, &push_completed]() {
            queue.push([]() {});
            push_completed = true;
        });
        producer.join();
        ASSERT_TRUE(push_completed.load());
    }
    // producer.join(); You can't do it here
}