// Demo: producers (customers) and consumers (warehouse workers) share a bounded queue.
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include "order_queue.hpp"

int main() {
    const int PRODUCERS = 2, CONSUMERS = 3, ORDERS_EACH = 5;
    OrderQueue queue(3);                       // small capacity to show blocking
    std::vector<std::thread> threads;

    for (int p = 0; p < PRODUCERS; p++)
        threads.emplace_back([&, p] {
            for (int i = 0; i < ORDERS_EACH; i++) {
                Order o;
                o.id = p * 100 + i;
                o.productId = 1 + (i % 4);
                o.quantity = 1 + (i % 3);
                o.priority = i % 5;
                queue.produce(o);
            }
        });

    const int total = PRODUCERS * ORDERS_EACH;
    std::atomic<int> taken{0};
    for (int c = 0; c < CONSUMERS; c++)
        threads.emplace_back([&, c] {
            while (taken.fetch_add(1) < total) {
                Order o = queue.consume();
                Logger::log("  Worker ", c, " processing order ", o.id);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                Logger::log("  Worker ", c, " finished order ", o.id);
            }
        });

    for (auto& t : threads) t.join();
    Logger::log("All orders processed.");
}