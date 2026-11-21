// Demo: the real threaded order queue running with each scheduling policy.
// Uses the same 5 jobs as scheduler.cpp, so results should match its simulation
// (small differences of a few hundredths come from real thread timing).
//
// Build: g++ -std=c++17 -pthread main_sched.cpp -o sched_queue
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <thread>
#include "sched_queue.hpp"

struct JobSpec { int id, arrival, burst, priority; };
static const std::vector<JobSpec> JOBS = {
    {1, 0, 6, 3}, {2, 1, 4, 1}, {3, 2, 9, 4}, {4, 3, 5, 2}, {5, 4, 2, 5}};
static const int UNIT_MS = 100;           // 1 work unit = 100 ms
static const int QUANTUM = 3;             // Round Robin time slice

struct Result { double wt, tat, rt, thr; };

static double units(Clock::duration d) {
    return std::chrono::duration<double, std::milli>(d).count() / UNIT_MS;
}

Result run(Policy p) {
    const int N = JOBS.size();
    SchedQueue q(p, N);
    std::vector<Task> done;
    auto t0 = Clock::now();

    std::thread customers([&] {
        for (auto& j : JOBS) {
            // wake slightly early so an arrival at the exact moment a slice ends is queued first
            std::this_thread::sleep_until(t0 + std::chrono::milliseconds(
                std::max(0, j.arrival * UNIT_MS - 30)));
            Task t; t.order.id = j.id; t.order.quantity = j.burst;
            t.order.priority = j.priority; t.remaining = j.burst;
            q.produce(t);
        }
    });

    std::thread worker([&] {
        Task t; int finished = 0;
        while (finished < N && q.consume(t)) {
            if (!t.started) { t.started = true; t.firstStart = Clock::now(); }
            int slice = (p == Policy::RR) ? std::min(QUANTUM, t.remaining) : t.remaining;
            std::this_thread::sleep_for(std::chrono::milliseconds(slice * UNIT_MS));
            t.remaining -= slice;
            if (t.remaining > 0) q.requeue(t);
            else { t.finished = Clock::now(); done.push_back(t); q.finish(); finished++; }
        }
    });
    customers.join(); worker.join();
    auto total = units(Clock::now() - t0);

    std::cout << "\n=== " << policyName(p) << (p == Policy::RR ? " (q=3)" : "") << " ===\n";
    std::cout << "Completion order: ";
    for (auto& t : done) std::cout << t.order.id << ' ';
    std::cout << "\n" << std::left << std::setw(6) << "Job" << std::setw(8) << "Burst"
              << std::setw(10) << "Waiting" << std::setw(12) << "Turnaround" << "Response\n";
    std::sort(done.begin(), done.end(), [](auto& a, auto& b) { return a.order.id < b.order.id; });
    Result r{0, 0, 0, 0};
    std::cout << std::fixed << std::setprecision(1);
    for (auto& t : done) {
        int arr = JOBS[t.order.id - 1].arrival;   // nominal arrival time (in units)
        double tat = units(t.finished - t0) - arr, rt = units(t.firstStart - t0) - arr;
        double wt = tat - t.order.quantity;
        std::cout << std::left << std::setw(6) << t.order.id << std::setw(8) << t.order.quantity
                  << std::setw(10) << wt << std::setw(12) << tat << rt << "\n";
        r.wt += wt; r.tat += tat; r.rt += rt;
    }
    r.wt /= N; r.tat /= N; r.rt /= N; r.thr = N / total;
    return r;
}

int main() {
    Result res[4]; Policy ps[4] = {Policy::FCFS, Policy::SJF, Policy::PRIORITY, Policy::RR};
    for (int i = 0; i < 4; i++) res[i] = run(ps[i]);

    std::cout << "\n=== Comparison (real threaded queue) ===\n" << std::fixed << std::setprecision(2)
              << std::left << std::setw(14) << "Algorithm" << std::setw(10) << "AvgWT"
              << std::setw(10) << "AvgTAT" << std::setw(10) << "AvgRT" << "Throughput\n";
    for (int i = 0; i < 4; i++)
        std::cout << std::left << std::setw(14) << policyName(ps[i]) << std::setw(10) << res[i].wt
                  << std::setw(10) << res[i].tat << std::setw(10) << res[i].rt << res[i].thr << "\n";
}
