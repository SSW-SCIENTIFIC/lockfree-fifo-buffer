#include <memory>
#include <future>

#include <gtest/gtest.h>

extern "C" {
#include "lockfree_fifo_buffer.h"
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

TEST(lockfree_fifo_buffer_initialize_test, it_is_initializable)
{
    auto const queue = reinterpret_cast<fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), 12));

    ASSERT_NE(queue, nullptr);

    queue->vptr->free(queue);
}

TEST(lockfree_fifo_buffer_initialize_test, it_has_enough_capacity)
{
    for (size_t i = 0; i < 65535; i += 1023) {
        auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), i));
        ASSERT_GE(queue->vptr->capacity(queue), i);
        queue->vptr->free(queue);
    }
}

TEST(lockfree_fifo_buffer_initialize_test, it_is_empty_after_initialization)
{
    for (size_t i = 0; i < 128; i++) {
        auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), i));
        ASSERT_TRUE(queue->vptr->is_empty(queue));
        ASSERT_EQ(queue->vptr->count(queue), 0);
        queue->vptr->free(queue);
    }
}

TEST(lockfree_fifo_buffer_enqueue_default_test, it_enqueues_an_element)
{
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), 14));
    
    const TestClass element(128);
    queue->vptr->enqueue_default(queue, &element, sizeof(element));

    ASSERT_EQ(queue->vptr->count(queue), 1);
}

TEST(lockfree_fifo_buffer_enqueue_default_test, it_enqueues_multiple_elements_until_full)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));
    
    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }

    ASSERT_GE(queue->vptr->count(queue), required_capacity);
}

TEST(lockfree_fifo_buffer_enqueue_default_test, it_cannot_enqueue_into_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }

    ASSERT_FALSE(queue->vptr->enqueue_default(queue, &element, sizeof(element)));
}

TEST(lockfree_fifo_buffer_enqueue_default_test, it_can_enqueue_after_dequeueing_from_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }
    queue->vptr->dequeue_default(queue, nullptr);

    ASSERT_TRUE(queue->vptr->enqueue_default(queue, &element, sizeof(element)));
}

TEST(lockfree_fifo_buffer_enqueue_test, it_enqueues_an_element)
{
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), 14));

    const TestClass element(128);
    queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);

    ASSERT_EQ(queue->vptr->count(queue), 1);
}

TEST(lockfree_fifo_buffer_enqueue_test, it_enqueues_multiple_elements_until_full)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);
    }

    ASSERT_GE(queue->vptr->count(queue), required_capacity);
}

TEST(lockfree_fifo_buffer_enqueue_test, it_cannot_enqueue_into_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);
    }

    ASSERT_FALSE(queue->vptr->enqueue_default(queue, &element, sizeof(element)));
}

TEST(lockfree_fifo_buffer_enqueue_test, it_can_enqueue_after_dequeueing_from_full_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);
    }
    queue->vptr->dequeue_default(queue, nullptr);

    ASSERT_TRUE(queue->vptr->enqueue_default(queue, &element, sizeof(element)));
}

TEST(lockfree_fifo_buffer_dequeue_default_test, it_cannot_dequeue_from_empty_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    ASSERT_FALSE(queue->vptr->dequeue_default(queue, nullptr));
}

TEST(lockfree_fifo_buffer_dequeue_default_test, it_cannot_dequeue_from_empty_queue_after_enqueueing)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }
    while (!queue->vptr->is_empty(queue)) {
        queue->vptr->dequeue_default(queue, nullptr);
    }

    ASSERT_FALSE(queue->vptr->dequeue_default(queue, nullptr));
}

TEST(lockfree_fifo_buffer_dequeue_default_test, it_dequeues_queued_element)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    queue->vptr->enqueue_default(queue, &element, sizeof(element));

    TestClass dequeued(0);
    queue->vptr->dequeue_default(queue, &dequeued);

    ASSERT_EQ(dequeued, element);
}

TEST(lockfree_fifo_buffer_dequeue_default_test, it_dequeues_queued_elements_order_by_first_in_first_out)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

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

TEST(lockfree_fifo_buffer_dequeue_test, it_cannot_dequeue_from_empty_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    ASSERT_FALSE(queue->vptr->dequeue(queue, nullptr, nullptr));
}

TEST(lockfree_fifo_buffer_dequeue_test, it_cannot_dequeue_from_empty_queue_after_enqueueing)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }
    while (!queue->vptr->is_empty(queue)) {
        queue->vptr->dequeue_default(queue, nullptr);
    }

    ASSERT_FALSE(queue->vptr->dequeue(queue, nullptr, nullptr));
}

TEST(lockfree_fifo_buffer_dequeue_test, it_dequeues_queued_element)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);

    TestClass dequeued(0);
    queue->vptr->dequeue(queue, &dequeued, default_copy);

    ASSERT_EQ(dequeued, element);
}

TEST(lockfree_fifo_buffer_dequeue_test, it_dequeues_queued_elements_order_by_first_in_first_out)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

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

TEST(lockfree_fifo_buffer_peek_test, it_cannot_peak_from_empty_queue)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    ASSERT_EQ(queue->vptr->peek_size(queue), 0);
    ASSERT_EQ(queue->vptr->peek(queue), nullptr);
}

TEST(lockfree_fifo_buffer_peek_test, it_peaks_enqueued_element)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

    const TestClass element(128);
    queue->vptr->enqueue(queue, &element, sizeof(element), default_copy);

    ASSERT_EQ(queue->vptr->peek_size(queue), sizeof(element));
    ASSERT_EQ(*reinterpret_cast<const TestClass *>(queue->vptr->peek(queue)), element);
}

TEST(lockfree_fifo_buffer_peek_test, it_peaks_element_which_is_dequeued_next)
{
    constexpr std::size_t required_capacity = 14;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

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

TEST(lockfree_fifo_buffer_count_test, it_increments_count_after_successfully_enqueued)
{
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), 14));

    const TestClass element(128);
    size_t previous = queue->vptr->count(queue);
    while (queue->vptr->enqueue_default(queue, &element, sizeof(element))) {
        const size_t current = queue->vptr->count(queue);
        ASSERT_EQ(current, previous + 1);
        previous = current;
    }
}

TEST(lockfree_fifo_buffer_count_test, it_does_not_increment_count_after_enqueue_failure)
{
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), 14));

    const TestClass element(128);
    while (queue->vptr->enqueue_default(queue, &element, sizeof(element))) {
        // do nothing
    }

    const size_t previous = queue->vptr->count(queue);
    EXPECT_FALSE(queue->vptr->enqueue_default(queue, &element, sizeof(element)));
    ASSERT_EQ(queue->vptr->count(queue), previous);
}

TEST(lockfree_fifo_buffer_count_test, it_decrements_count_after_successfully_dequeued)
{
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), 14));

    const TestClass element(128);
    while (queue->vptr->enqueue_default(queue, &element, sizeof(element))) {
        // do nothing
    }

    size_t previous = queue->vptr->count(queue);
    while (queue->vptr->dequeue_default(queue, nullptr)) {
        const size_t current = queue->vptr->count(queue);
        ASSERT_EQ(current, previous - 1);
        previous = current;
    }
}

TEST(lockfree_fifo_buffer_count_test, it_does_not_decrement_count_after_dequeue_failure)
{
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), 14));

    const size_t previous = queue->vptr->count(queue);
    EXPECT_FALSE(queue->vptr->dequeue_default(queue, nullptr));
    ASSERT_EQ(queue->vptr->count(queue), previous);
}

TEST(lockfree_fifo_buffer_count_test, it_returns_count_consistently_by_enqueueing_and_dequeueing_alternately)
{
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), 0xFF));

    const TestClass element(0);
    while (!queue->vptr->is_full(queue)) {
        queue->vptr->enqueue_default(queue, &element, sizeof(element));
    }

    for (size_t i = 0; i < queue->vptr->capacity(queue); i++) {
        const size_t count_full = queue->vptr->count(queue);
        queue->vptr->dequeue_default(queue, nullptr);
        ASSERT_EQ(queue->vptr->count(queue), count_full - 1);

        queue->vptr->enqueue_default(queue, &element, sizeof(element));
        ASSERT_EQ(queue->vptr->count(queue), count_full);
    }
}

TEST(lockfree_fifo_buffer_contensivity_test, it_never_contensive_when_single_reader_and_single_writer)
{
    constexpr std::size_t required_capacity = 2048;
    auto const queue = reinterpret_cast<struct fifo_buffer *>(lockfree_fifo_buffer_new(sizeof(TestClass), required_capacity));

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
