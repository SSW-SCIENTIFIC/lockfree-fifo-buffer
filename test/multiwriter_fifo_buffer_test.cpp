#include <memory>
#include <future>

#include <gtest/gtest.h>

#include <unordered_set>
#include <mutex>

extern "C" {
#include "multiwriter_fifo_buffer.h"
}

class TestClass {
private:
    std::size_t dummy_;
public:
    explicit TestClass(std::size_t size): dummy_(size)
    {
        // do nothing
    }

    TestClass(const TestClass &rhs) = default;
    TestClass(TestClass &&rhs) = default;
    TestClass &operator=(const TestClass &rhs) = default;
    TestClass &operator=(TestClass &&rhs) = default;

    bool operator==(const TestClass &rhs) const { return this->dummy_ == rhs.dummy_; }
    [[nodiscard]] std::size_t dummy() const { return this->dummy_; }
};

const auto default_copy = [] (void *to, const void *from, size_t) -> void * {
    *reinterpret_cast<TestClass *>(to) = *reinterpret_cast<const TestClass *>(from);
    return to;
};

TEST(multiwriter_fifo_buffer_initialize_test, it_is_initializable)
{
    auto const queue = reinterpret_cast<fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), 12));

    ASSERT_NE(queue, nullptr);

    queue->vptr->free(queue);
}

TEST(multiwriter_fifo_buffer_initialize_test, it_has_enough_capacity)
{
    for (size_t i = 0; i < 65535; i += 1023) {
        auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), i));
        ASSERT_GE(queue->vptr->capacity(queue), i);
        queue->vptr->free(queue);
    }
}

TEST(multiwriter_fifo_buffer_initialize_test, it_is_empty_after_initialization)
{
    for (size_t i = 0; i < 128; i++) {
        auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), i));
        ASSERT_TRUE(queue->vptr->is_empty(queue));
        ASSERT_EQ(queue->vptr->count(queue), 0);
        queue->vptr->free(queue);
    }
}

TEST(multiwriter_fifo_buffer_enqueue_default_test, it_enqueues_an_element)
{
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), 14));
    
    const TestClass element(128);
    queue->vptr->enqueue_default(queue, &element, sizeof(element));

    ASSERT_EQ(queue->vptr->count(queue), 1);
}

TEST(multiwriter_fifo_buffer_enqueue_default_test, it_enqueues_multiple_elements_until_full)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));
    
    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }

    ASSERT_GE(queue->vptr->count(queue), required_capacity);
}

TEST(multiwriter_fifo_buffer_enqueue_default_test, it_cannot_enqueue_into_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }

    ASSERT_FALSE(queue->vptr->enqueue_default(queue, &element, sizeof(element)));
}

TEST(multiwriter_fifo_buffer_enqueue_default_test, it_can_enqueue_after_dequeueing_from_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }
    queue->vptr->dequeue_default(queue, nullptr);

    ASSERT_TRUE(queue->vptr->enqueue_default(queue, &element, sizeof(element)));
}

TEST(multiwriter_fifo_buffer_enqueue_test, it_enqueues_an_element)
{
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), 14));

    const TestClass element(128);
    queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);

    ASSERT_EQ(queue->vptr->count(queue), 1);
}

TEST(multiwriter_fifo_buffer_enqueue_test, it_enqueues_multiple_elements_until_full)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);
    }

    ASSERT_GE(queue->vptr->count(queue), required_capacity);
}

TEST(multiwriter_fifo_buffer_enqueue_test, it_cannot_enqueue_into_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);
    }

    ASSERT_FALSE(queue->vptr->enqueue_default(queue, &element, sizeof(element)));
}

TEST(multiwriter_fifo_buffer_enqueue_test, it_can_enqueue_after_dequeueing_from_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full((struct fifo_buffer *)queue)) {
        queue->vptr->enqueue((struct fifo_buffer *)queue, &element, sizeof(element), default_copy);
    }
    queue->vptr->dequeue_default((struct fifo_buffer *)queue, nullptr);

    ASSERT_TRUE(queue->vptr->enqueue_default((struct fifo_buffer *)queue, &element, sizeof(element)));
}

TEST(multiwriter_fifo_buffer_try_enqueue_default_test, it_enqueues_an_element)
{
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), 14));
    
    const TestClass element(128);
    queue->vptr->try_enqueue_default((struct fifo_buffer *)queue, &element, sizeof(element));

    ASSERT_EQ(queue->vptr->count((struct fifo_buffer *)queue), 1);
}

TEST(multiwriter_fifo_buffer_try_enqueue_default_test, it_enqueues_multiple_elements_until_full)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));
    
    const TestClass element(128);
    while (!queue->vptr->is_full((struct fifo_buffer *)queue)) {
        queue->vptr->try_enqueue_default((struct fifo_buffer *)queue, &element, sizeof(element));
    }

    ASSERT_GE(queue->vptr->count((struct fifo_buffer *)queue), required_capacity);
}

TEST(multiwriter_fifo_buffer_try_enqueue_default_test, it_cannot_try_enqueue_into_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full((struct fifo_buffer *)queue)) {
        queue->vptr->try_enqueue_default((struct fifo_buffer *)queue, &element, sizeof(element));
    }

    ASSERT_FALSE(queue->vptr->try_enqueue_default((struct fifo_buffer *)queue, &element, sizeof(element)));
}

TEST(multiwriter_fifo_buffer_try_enqueue_default_test, it_can_try_enqueue_after_dequeueing_from_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full((struct fifo_buffer *)queue)) {
        queue->vptr->try_enqueue_default((struct fifo_buffer *)queue, &element, sizeof(element));
    }
    queue->vptr->dequeue_default((struct fifo_buffer *)queue, nullptr);

    ASSERT_TRUE(queue->vptr->try_enqueue_default((struct fifo_buffer *)queue, &element, sizeof(element)));
}

TEST(multiwriter_fifo_buffer_try_enqueue_default_test, it_fails_to_enqueue_when_contensive_condition)
{
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), 4096));

    std::vector<std::future<void>> tasks;
    std::atomic<bool> stopped(false);
    std::atomic<bool> try_failed(false);

    tasks.push_back(std::async(std::launch::async, [&stopped, queue] () {
        while (!stopped.load()) {
            queue->vptr->dequeue_default((fifo_buffer *)queue, nullptr);
        }
    }));
    for (size_t i = 0; i < 32; i++) {
        tasks.push_back(std::async(std::launch::async, [&stopped, &try_failed, queue] () {
            for (size_t i = 0; i < 65535; i++) {
                if (stopped.load()) {
                    break;
                }
                const TestClass element(i);
                if (!queue->vptr->try_enqueue_default((fifo_buffer *)queue, &element, sizeof(element))) {
                    try_failed.store(true);
                    stopped.store(true);
                    break;
                }
            }
        }));
    }

    for (auto &task: tasks) {
        task.wait();
    }

    ASSERT_TRUE(try_failed.load());
}

TEST(multiwriter_fifo_buffer_try_enqueue_test, it_enqueues_an_element)
{
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), 14));

    const TestClass element(128);
    queue->vptr->try_enqueue((struct fifo_buffer *)queue, &element, sizeof(element), default_copy);

    ASSERT_EQ(queue->vptr->count((struct fifo_buffer *)queue), 1);
}

TEST(multiwriter_fifo_buffer_try_enqueue_test, it_enqueues_multiple_elements_until_full)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full((struct fifo_buffer *)queue)) {
        queue->vptr->try_enqueue((struct fifo_buffer *)queue, &element, sizeof(element), default_copy);
    }

    ASSERT_GE(queue->vptr->count((struct fifo_buffer *)queue), required_capacity);
}

TEST(multiwriter_fifo_buffer_try_enqueue_test, it_cannot_try_enqueue_into_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full((struct fifo_buffer *)queue)) {
        queue->vptr->try_enqueue((struct fifo_buffer *)queue, &element, sizeof(element), default_copy);
    }

    ASSERT_FALSE(queue->vptr->try_enqueue_default((struct fifo_buffer *)queue, &element, sizeof(element)));
}

TEST(multiwriter_fifo_buffer_try_enqueue_test, it_can_try_enqueue_after_dequeueing_from_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full((struct fifo_buffer *)queue)) {
        queue->vptr->try_enqueue((struct fifo_buffer *)queue, &element, sizeof(element), default_copy);
    }
    queue->vptr->dequeue_default((struct fifo_buffer *)queue, nullptr);

    ASSERT_TRUE(queue->vptr->try_enqueue_default((struct fifo_buffer *)queue, &element, sizeof(element)));
}

TEST(multiwriter_fifo_buffer_try_enqueue_test, it_fails_to_enqueue_when_contensive_condition)
{
    auto const queue = reinterpret_cast<struct multiwriter_fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), 4096));

    std::vector<std::future<void>> tasks;
    std::atomic<bool> stopped(false);
    std::atomic<bool> try_failed(false);

    tasks.push_back(std::async(std::launch::async, [&stopped, queue] () {
        while (!stopped.load()) {
            queue->vptr->dequeue_default((fifo_buffer *)queue, nullptr);
        }
    }));
    for (size_t i = 0; i < 32; i++) {
        tasks.push_back(std::async(std::launch::async, [&stopped, &try_failed, queue] () {
            for (size_t i = 0; i < 65535; i++) {
                if (stopped.load()) {
                    break;
                }
                const TestClass element(i);
                if (!queue->vptr->try_enqueue((fifo_buffer *)queue, &element, sizeof(element), default_copy)) {
                    try_failed.store(true);
                    stopped.store(true);
                    break;
                }
            }
        }));
    }

    for (auto &task: tasks) {
        task.wait();
    }

    ASSERT_TRUE(try_failed.load());
}

TEST(multiwriter_fifo_buffer_dequeue_default_test, it_cannot_dequeue_from_empty_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    ASSERT_FALSE(queue->vptr->dequeue_default(queue, nullptr));
}

TEST(multiwriter_fifo_buffer_dequeue_default_test, it_cannot_dequeue_from_empty_queue_after_enqueueing)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }
    while (!queue->vptr->is_empty(queue)) {
        queue->vptr->dequeue_default(queue, nullptr);
    }

    ASSERT_FALSE(queue->vptr->dequeue_default(queue, nullptr));
}

TEST(multiwriter_fifo_buffer_dequeue_default_test, it_dequeues_queued_element)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    queue->vptr->enqueue_default(queue, &element, sizeof(element));

    TestClass dequeued(0);
    queue->vptr->dequeue_default(queue, &dequeued);

    ASSERT_EQ(dequeued, element);
}

TEST(multiwriter_fifo_buffer_dequeue_default_test, it_dequeues_queued_elements_order_by_first_in_first_out)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    std::vector<TestClass> elements;
    std::vector<TestClass> dequeues;
    elements.reserve(65535);
    dequeues.reserve(65535);

    while (elements.size() != elements.capacity()) {
        for (size_t i = elements.size(); i < elements.capacity(); i++) {
            if (queue->vptr->is_full(queue)) {
                break;
            }
            elements.emplace_back(i);
            queue->vptr->enqueue_default(queue, &elements.at(i), sizeof(elements.at(i)));
        }
        while (!queue->vptr->is_empty(queue)) {
            TestClass dequeued(0);
            queue->vptr->dequeue_default(queue, &dequeued);
            dequeues.push_back(dequeued);
        }
    }

    ASSERT_EQ(dequeues, elements);
}

TEST(multiwriter_fifo_buffer_dequeue_test, it_cannot_dequeue_from_empty_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    ASSERT_FALSE(queue->vptr->dequeue(queue, nullptr, nullptr));
}

TEST(multiwriter_fifo_buffer_dequeue_test, it_cannot_dequeue_from_empty_queue_after_enqueueing)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }
    while (!queue->vptr->is_empty(queue)) {
        queue->vptr->dequeue_default(queue, nullptr);
    }

    ASSERT_FALSE(queue->vptr->dequeue(queue, nullptr, nullptr));
}

TEST(multiwriter_fifo_buffer_dequeue_test, it_dequeues_queued_element)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);

    TestClass dequeued(0);
    queue->vptr->dequeue(queue, &dequeued, default_copy);

    ASSERT_EQ(dequeued, element);
}

TEST(multiwriter_fifo_buffer_dequeue_test, it_dequeues_queued_elements_order_by_first_in_first_out)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    std::vector<TestClass> elements;
    std::vector<TestClass> dequeues;
    elements.reserve(65535);
    dequeues.reserve(65535);

    while (elements.size() != elements.capacity()) {
        for (size_t i = elements.size(); i < elements.capacity(); i++) {
            if (queue->vptr->is_full(queue)) {
                break;
            }
            elements.emplace_back(i);
            queue->vptr->enqueue(queue, &elements.at(i), sizeof(elements.at(i)), default_copy);
        }
        while (!queue->vptr->is_empty(queue)) {
            TestClass dequeued(0);
            queue->vptr->dequeue(queue, &dequeued, default_copy);
            dequeues.push_back(dequeued);
        }
    }

    ASSERT_EQ(dequeues, elements);
}

TEST(multiwriter_fifo_buffer_peek_test, it_cannot_peak_from_empty_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    ASSERT_EQ(queue->vptr->peek_size(queue), 0);
    ASSERT_EQ(queue->vptr->peek(queue), nullptr);
}

TEST(multiwriter_fifo_buffer_peek_test, it_peaks_enqueued_element)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);

    ASSERT_EQ(queue->vptr->peek_size(queue), sizeof(element));
    ASSERT_EQ(*reinterpret_cast<const TestClass *>(queue->vptr->peek(queue)), element);
}

TEST(multiwriter_fifo_buffer_peek_test, it_peaks_element_which_is_dequeued_next)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    std::vector<TestClass> elements;
    elements.reserve(65535);

    while (elements.size() != elements.capacity()) {
        for (size_t i = elements.size(); i < elements.capacity(); i++) {
            if (queue->vptr->is_full(queue)) {
                break;
            }
            elements.emplace_back(i);
            queue->vptr->enqueue(queue, &elements.at(i), sizeof(elements.at(i)), default_copy);
        }
        while (!queue->vptr->is_empty(queue)) {
            const auto element_size = queue->vptr->peek_size(queue);
            const auto *const element = reinterpret_cast<const TestClass *>(queue->vptr->peek(queue));

            TestClass dequeued(0);
            queue->vptr->dequeue_default(queue, &dequeued);

            ASSERT_EQ(*element, dequeued);
        }
    }
}

TEST(multiwriter_fifo_buffer_contensivity_test, it_never_contensive_when_single_reader_and_single_writer)
{
    constexpr std::size_t required_capacity = 2048;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(multiwriter_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const size_t tail = 65536 * 16;

    auto consumer = std::async(std::launch::async, [queue] () {
        for (size_t i = 0; i < tail; i++) {
            TestClass element(0);
            while (!queue->vptr->dequeue_default(queue, &element)) {
                // block until successfully dequeued
            }
            ASSERT_EQ(element.dummy(), i);
        }
    });
    auto producer = std::async(std::launch::async, [queue] () {
        for (size_t i = 0; i < tail; i++) {
            TestClass element(i);
            while (!queue->vptr->enqueue_default(queue, &element, sizeof(element))) {
                // block until successfully enqueued
            }
        }
    });

    producer.wait();
    consumer.wait();
}