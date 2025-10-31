#include <gtest/gtest.h>

#include "queue/bounded_queue.hpp"

using namespace dispatcher::queue;

TEST(BoundedQueueTest, constructor) {
    EXPECT_THROW(BoundedQueue(-1), std::invalid_argument);
    EXPECT_NO_THROW(BoundedQueue(1));
}

TEST(BoundedQueueTest, pushPop) {
    BoundedQueue queue(2);
    EXPECT_FALSE(queue.try_pop().has_value());

    bool executed = false;
    queue.push([&executed]() { executed = true; });
    auto task = queue.try_pop();
    ASSERT_TRUE(task.has_value());
    ASSERT_FALSE(executed);
    (*task)();
    EXPECT_TRUE(executed);
}

TEST(BoundedQueueTest, fifo) {
    BoundedQueue queue(5);
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

TEST(BoundedQueueTest, capacityLimit) {
    const int capacity = 3;
    BoundedQueue queue(capacity);

    for (int i = 0; i < capacity; ++i) {
        queue.push([]() {});
    }

    std::atomic<bool> push_completed{false};
    std::thread producer([&queue, &push_completed]() {
        queue.push([]() {});  // BLOCKED
        push_completed = true;
    });
    ASSERT_FALSE(push_completed.load());

    auto task = queue.try_pop();
    EXPECT_TRUE(task.has_value());

    ASSERT_FALSE(push_completed.load());
    producer.join();
    EXPECT_TRUE(push_completed.load());
}

TEST(BoundedQueueTest, shutdown) {
    std::atomic<bool> push_unblocked{false};
    std::atomic<bool> push_started{false};
    std::thread blocked_producer;

    {
        BoundedQueue queue(2);

        queue.push([]() {});
        queue.push([]() {});

        blocked_producer = std::thread([&queue, &push_unblocked, &push_started]() {
            push_started = true;
            queue.push([]() {});
            push_unblocked = true;
        });

        while (!push_started.load()) {
            std::this_thread::yield();
        }
        ASSERT_FALSE(push_unblocked.load());
    }

    blocked_producer.join();
    EXPECT_TRUE(push_unblocked.load());
}

// здесь ваш код