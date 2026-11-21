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
    int id = -1;          
    int productId = 0;    
    int quantity = 1;     
    int priority = 0;     
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
