#pragma once
// Order Queue is a classic PRODUCER-CONSUMER bounded buffer.
// Producers : are customers placing orders
// Consumers:  warehouse worker threads
// empty_ : available queue slots
// full_ : orders waiting to be processed
// mutex  : protects the queue

#include <mutex>
#include <queue>
#include <condition_variable>
#include <cstddef>
#include "common.hpp"

// Counting semaphore built from mutex + condition variable (works in C++17).
class Semaphore {
public:
    explicit Semaphore(std::ptrdiff_t c) : count_(c) {}
    void acquire() {                       // P / wait
        std::unique_lock<std::mutex> l(m_);
        cv_.wait(l, [&] { return count_ > 0; });
        --count_;
    }
    void release() {                       // V / signal
        { std::lock_guard<std::mutex> l(m_); ++count_; }
        cv_.notify_one();
    }
private:
    std::mutex m_;
    std::condition_variable cv_;
    std::ptrdiff_t count_;
};

class OrderQueue {
public:
    explicit OrderQueue(std::ptrdiff_t capacity): empty_(capacity), full_(0) {}

    void produce(Order o) { //placing order
        // Wait until there is space in the bounded buffer
        empty_.acquire();
        {
            std::lock_guard<std::mutex> g(m_);
            o.state = OrderState::QUEUED;
            q_.push(o);
            Logger::log(
                "Order ", o.id,
                " queued | Product: ", o.productId,
                " | Quantity: ", o.quantity,
                " | Priority: ", o.priority
            );
        }
        // Signal that a new order is given
        full_.release();
    }

    Order consume() { // Warehouse worker taking up an order
        full_.acquire(); // Wait until an order is placed
        Order o;
        {
            std::lock_guard<std::mutex> g(m_);
            o = q_.front();
            q_.pop();}
        empty_.release(); // Signal that a queue slot is freed

        Logger::log(
            "Order ", o.id,
            " dequeued for processing");
        return o;
    }

private:
    std::mutex m_;
    // FIFO queue.
    // Scheduler will replace this queue for FCFS, SJF, Priority, and Round Robin.
    std::queue<Order> q_;
    Semaphore empty_;
    Semaphore full_;
};
