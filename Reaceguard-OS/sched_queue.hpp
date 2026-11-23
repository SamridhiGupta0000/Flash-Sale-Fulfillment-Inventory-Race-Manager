#pragma once
// Scheduled order queue: the FIFO queue replaced by a pluggable scheduling policy.
// Same bounded-buffer idea (semaphores + mutex), but consume() PICKS the next order
// using FCFS, SJF, Priority or Round Robin.
//
// Burst time of an order = its quantity (units of packing work).
// A queue slot is held until the order is completely finished (finish()),
// so a Round Robin re-queue never needs a free slot and can never deadlock.

#include <chrono>
#include <mutex>
#include <vector>
#include "order_queue.hpp"   // reuses Semaphore, Order, Logger

enum class Policy { FCFS, SJF, PRIORITY, RR };

inline const char* policyName(Policy p) {
    switch (p) {
        case Policy::FCFS:     return "FCFS";
        case Policy::SJF:      return "SJF";
        case Policy::PRIORITY: return "Priority";
        case Policy::RR:       return "RoundRobin";
    }
    return "?";
}

using Clock = std::chrono::steady_clock;

struct Task {
    Order order;
    int remaining = 0;               // work units still to do
    long seq = 0;                    // arrival sequence (FCFS tie-break)
    bool started = false;
    Clock::time_point enqueued, firstStart, finished;
};

class SchedQueue {
public:
    SchedQueue(Policy p, std::ptrdiff_t capacity)
        : policy_(p), empty_(capacity), full_(0) {}

    // Customer places an order (blocks if the system is full)
    void produce(Task t) {
        empty_.acquire();
        {
            std::lock_guard<std::mutex> g(m_);
            t.seq = nextSeq_++;
            t.enqueued = Clock::now();
            t.order.state = OrderState::QUEUED;
            tasks_.push_back(t);
        }
        full_.release();
    }

    // Round Robin: unfinished order goes to the back (keeps its slot)
    void requeue(Task t) {
        {
            std::lock_guard<std::mutex> g(m_);
            t.seq = nextSeq_++;
            tasks_.push_back(t);
        }
        full_.release();
    }

    // Worker takes the next order chosen by the policy.
    // Returns false when the queue was closed and nothing is left.
    bool consume(Task& out) {
        full_.acquire();
        std::lock_guard<std::mutex> g(m_);
        if (tasks_.empty()) return false;
        size_t best = 0;
        for (size_t i = 1; i < tasks_.size(); i++)
            if (better(tasks_[i], tasks_[best])) best = i;
        out = tasks_[best];
        tasks_.erase(tasks_.begin() + best);
        return true;
    }

    void finish() { empty_.release(); }              // order completely done
    void close(int workers) { for (int i = 0; i < workers; i++) full_.release(); }
    Policy policy() const { return policy_; }

private:
    bool better(const Task& a, const Task& b) const {
        switch (policy_) {
            case Policy::SJF:
                if (a.order.quantity != b.order.quantity) return a.order.quantity < b.order.quantity;
                break;
            case Policy::PRIORITY:   // lower number = higher priority
                if (a.order.priority != b.order.priority) return a.order.priority < b.order.priority;
                break;
            default: break;          // FCFS and RR: arrival order
        }
        return a.seq < b.seq;
    }

    Policy policy_;
    std::mutex m_;
    std::vector<Task> tasks_;
    long nextSeq_ = 0;
    Semaphore empty_, full_;
};
