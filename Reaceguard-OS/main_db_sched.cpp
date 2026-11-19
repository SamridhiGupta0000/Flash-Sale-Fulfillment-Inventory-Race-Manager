// Full system: Producer-Consumer + CPU Scheduling policy + PostgreSQL transactions
//
//  Customers (producers) -> INSERT order in DB -> SchedQueue
//  Workers   (consumers) -> SchedQueue picks the next order by the chosen POLICY
//                           (FCFS / SJF / Priority / Round Robin) -> work -> DB transaction
//
// Build: g++ -std=c++17 -pthread -Wno-deprecated-declarations main_db_sched.cpp -o pbl_sched -lpqxx -lpq
// Run  : ./pbl_sched "dbname=pbl user=postgres password=pbl123 host=localhost" fcfs|sjf|priority|rr [customers] [orders_each] [workers]

#include <algorithm>
#include <cstdlib>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <vector>
#include <pqxx/pqxx>
#include "sched_queue.hpp"

static const int WAREHOUSE_ID = 1;     // WH-DEL
static const int UNIT_MS = 100;        // 1 unit of quantity = 100 ms of packing work
static const int QUANTUM = 1;          // Round Robin slice (units)

// ---------- PRODUCER: customer places an order ----------
static int g_total = 0;                // total orders (for progress messages)

void customerThread(const std::string& conninfo, SchedQueue& queue,
                    int threadIdx, int numOrders) {
    int customerId = 1 + (threadIdx - 1) % 5;   // seed data has customers 1..5
    try {
        pqxx::connection conn(conninfo);
        for (int i = 0; i < numOrders; i++) {
            int productId = 1 + (customerId + i) % 4;
            int quantity  = 1 + (i % 3);
            int priority  = (customerId + i) % 5;      // lower number = more urgent

            pqxx::work tx(conn);
            pqxx::result r = tx.exec_params(
                "INSERT INTO orders(customer_id, total_amount) "
                "SELECT $1, price * $3 FROM products WHERE product_id = $2 "
                "RETURNING order_id", customerId, productId, quantity);
            int orderId = r[0][0].as<int>();
            tx.exec_params(
                "INSERT INTO order_items(order_id, product_id, quantity, unit_price) "
                "SELECT $1, product_id, $3, price FROM products WHERE product_id = $2",
                orderId, productId, quantity);
            tx.commit();

            Task t;
            t.order.id = orderId;  t.order.productId = productId;
            t.order.quantity = quantity;  t.order.priority = priority;
            t.remaining = quantity;                    // burst time = quantity
            queue.produce(t);
            Logger::log("Order ", orderId, " queued | Product: ", productId,
                        " | Burst(qty): ", quantity, " | Priority: ", priority);
        }
    } catch (const std::exception& e) {
        Logger::log("Customer ", customerId, " ERROR: ", e.what());
    }
}

// ---------- CONSUMER: warehouse worker processes an order ----------
void processOrder(pqxx::connection& conn, const Order& o, int workerId) {
    pqxx::work tx(conn);
    tx.exec("SET TRANSACTION ISOLATION LEVEL READ COMMITTED");

    // Lock the inventory row so two workers can't sell the same stock
    pqxx::result r = tx.exec_params(
        "SELECT warehouse_id, stock_quantity FROM inventory "
        "WHERE product_id = $1 ORDER BY stock_quantity DESC LIMIT 1 FOR UPDATE",
        o.productId);                                   // multi-warehouse: use the one with most stock
    int wh = r.empty() ? WAREHOUSE_ID : r[0][0].as<int>();
    int stock = r.empty() ? 0 : r[0][1].as<int>();

    if (stock >= o.quantity) {
        tx.exec_params(
            "UPDATE inventory SET stock_quantity = stock_quantity - $1 "
            "WHERE product_id = $2 AND warehouse_id = $3",
            o.quantity, o.productId, wh);
        tx.exec_params("UPDATE orders SET order_status = 'CONFIRMED' WHERE order_id = $1", o.id);
        tx.exec_params(
            "INSERT INTO payments(order_id, amount, payment_status, paid_at) "
            "SELECT order_id, total_amount, 'PAID', CURRENT_TIMESTAMP FROM orders WHERE order_id = $1",
            o.id);
        tx.exec_params(
            "INSERT INTO shipments(order_id, warehouse_id, shipment_status) VALUES ($1, $2, 'PACKED')",
            o.id, wh);
        tx.commit();
        Logger::log("  [Worker ", workerId, "] Order ", o.id, " CONFIRMED (stock was ", stock,
                    ", now ", stock - o.quantity, ")");
    } else {
        tx.exec_params("UPDATE orders SET order_status = 'FAILED' WHERE order_id = $1", o.id);
        tx.commit();
        Logger::log("  [Worker ", workerId, "] Order ", o.id, " FAILED (stock ", stock,
                    " < requested ", o.quantity, ")");
    }
}

void workerThread(const std::string& conninfo, SchedQueue& queue,
                  std::atomic<int>& finished, int workerId) {
    try {
        pqxx::connection conn(conninfo);
        Task t;
        while (queue.consume(t)) {                     // policy chooses the order
            bool rr = (queue.policy() == Policy::RR);
            int slice = rr ? std::min(QUANTUM, t.remaining) : t.remaining;
            Logger::log("[Worker ", workerId, "] picked order ", t.order.id,
                        " (qty ", t.order.quantity, ", prio ", t.order.priority,
                        ", left ", t.remaining, ")");
            std::this_thread::sleep_for(std::chrono::milliseconds(slice * UNIT_MS));
            t.remaining -= slice;
            if (t.remaining > 0) { queue.requeue(t); continue; }   // Round Robin
            processOrder(conn, t.order, workerId);                 // DB transaction
            queue.finish();
            int f = ++finished;
            if (f % 50 == 0) Logger::log(">>> Progress: ", f, " / ", g_total, " orders done");
        }
    } catch (const std::exception& e) {
        Logger::log("Worker ", workerId, " ERROR: ", e.what());
    }
}

int main(int argc, char* argv[]) {
    std::string conninfo = (argc > 1) ? argv[1] : "dbname=pbl user=postgres host=localhost";
    std::string pol = (argc > 2) ? argv[2] : "fcfs";
    Policy policy = pol == "sjf" ? Policy::SJF : pol == "priority" ? Policy::PRIORITY
                  : pol == "rr" ? Policy::RR : Policy::FCFS;

    // Dataset size (defaults give roughly 40 seconds): customers x orders-each, workers
    const int CUSTOMERS    = (argc > 3) ? std::atoi(argv[3]) : 8;
    const int ORDERS_EACH  = (argc > 4) ? std::atoi(argv[4]) : 50;
    const int WORKERS      = (argc > 5) ? std::atoi(argv[5]) : 2;
    const int total = CUSTOMERS * ORDERS_EACH;
    g_total = total;
    auto startTime = std::chrono::steady_clock::now();

    SchedQueue queue(policy, total);                   // room for all orders -> backlog builds up
    std::atomic<int> finished{0};
    std::vector<std::thread> customers, workers;
    Logger::log("=== Scheduling policy: ", policyName(policy), " ===");

    for (int c = 1; c <= CUSTOMERS; c++)
        customers.emplace_back(customerThread, std::cref(conninfo), std::ref(queue), c, ORDERS_EACH);
    for (int w = 1; w <= WORKERS; w++)
        workers.emplace_back(workerThread, std::cref(conninfo), std::ref(queue), std::ref(finished), w);

    for (auto& t : customers) t.join();
    while (finished < total) std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.close(WORKERS);                              // wake workers so they exit
    for (auto& t : workers) t.join();

    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
    Logger::log("\nAll ", total, " orders processed with ", policyName(policy), " in ",
                (int)secs, " seconds (", WORKERS, " workers).");
    return 0;
}
