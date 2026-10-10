#pragma once
// Shared parts: Order, Logger.
#include <iostream>
#include <mutex>
#include <sstream>

enum class OrderState {
    NEW,
    QUEUED,
    PROCESSING,
    DONE,
    FAILED
};

struct Order {
    int id = -1;          // orders.order_id
    int productId = 0;    // orders.product_id
    int quantity = 1;     // orders.quantity
    int priority = 0;     // OS scheduler priority
    OrderState state = OrderState::NEW;
};

class Logger {
public:
    template <typename... Args>
    static void log(Args&&... args) {
        std::ostringstream os;
        (os << ... << args);

        std::lock_guard<std::mutex> g(mutex());
        std::cout << os.str() << '\n';}

private:
    static std::mutex& mutex() {
        static std::mutex m;
        return m;}
};