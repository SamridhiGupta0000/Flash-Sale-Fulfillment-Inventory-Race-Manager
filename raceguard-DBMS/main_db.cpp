// Full demo: Producer-Consumer (OS) + PostgreSQL transactions (DBMS)
//
//  Customers (producer threads)  -> INSERT order into DB (status PENDING) -> push to OrderQueue
//  Workers   (consumer threads)  -> pop order -> BEGIN; SELECT ... FOR UPDATE; check stock;
//                                   deduct stock, CONFIRM order, create payment + shipment; COMMIT
//                                   (not enough stock -> order FAILED)
//
// Build: g++ -std=c++20 -pthread main_db.cpp -o pbl_db -lpqxx -lpq
// Run  : ./pbl_db "dbname=pbl user=postgres password=YOURPASS host=localhost"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <pqxx/pqxx>
#include "order_queue.hpp"

static const int WAREHOUSE_ID = 1;   // WH-DEL (the one that has stock)

// ---------- PRODUCER: customer places an order ----------
void customerThread(const std::string& conninfo, OrderQueue& queue,
                    int customerId, int numOrders) {
    try {
        pqxx::connection conn(conninfo);          // one connection per thread
        for (int i = 0; i < numOrders; i++) {
            int productId = 1 + (customerId + i) % 4;
            int quantity  = 1 + (i % 3);

            pqxx::work tx(conn);
            // order row (total = price * quantity)
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

            Order o;
            o.id = orderId;
            o.productId = productId;
            o.quantity = quantity;
            o.priority = i % 5;
            queue.produce(o);                     // blocks if queue is full
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
        "SELECT stock_quantity FROM inventory "
        "WHERE product_id = $1 AND warehouse_id = $2 FOR UPDATE",
        o.productId, WAREHOUSE_ID);
    int stock = r.empty() ? 0 : r[0][0].as<int>();

    if (stock >= o.quantity) {
        tx.exec_params(
            "UPDATE inventory SET stock_quantity = stock_quantity - $1 "
            "WHERE product_id = $2 AND warehouse_id = $3",
            o.quantity, o.productId, WAREHOUSE_ID);
        tx.exec_params("UPDATE orders SET order_status = 'CONFIRMED' WHERE order_id = $1", o.id);
        tx.exec_params(
            "INSERT INTO payments(order_id, amount, payment_status, paid_at) "
            "SELECT order_id, total_amount, 'PAID', CURRENT_TIMESTAMP FROM orders WHERE order_id = $1",
            o.id);
        tx.exec_params(
            "INSERT INTO shipments(order_id, warehouse_id, shipment_status) VALUES ($1, $2, 'PACKED')",
            o.id, WAREHOUSE_ID);
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

void workerThread(const std::string& conninfo, OrderQueue& queue,
                  std::atomic<int>& taken, int total, int workerId) {
    try {
        pqxx::connection conn(conninfo);
        while (taken.fetch_add(1) < total) {
            Order o = queue.consume();            // blocks if queue is empty
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // simulate packing
            processOrder(conn, o, workerId);
        }
    } catch (const std::exception& e) {
        Logger::log("Worker ", workerId, " ERROR: ", e.what());
    }
}

int main(int argc, char* argv[]) {
    std::string conninfo = (argc > 1) ? argv[1] : "dbname=pbl user=postgres host=localhost";
    const int CUSTOMERS = 3, ORDERS_EACH = 4, WORKERS = 3;
    const int total = CUSTOMERS * ORDERS_EACH;

    OrderQueue queue(3);                          // small capacity -> shows blocking
    std::atomic<int> taken{0};
    std::vector<std::thread> threads;

    for (int c = 1; c <= CUSTOMERS; c++)          // customer ids 1..3 exist in seed data
        threads.emplace_back(customerThread, std::cref(conninfo), std::ref(queue), c, ORDERS_EACH);
    for (int w = 1; w <= WORKERS; w++)
        threads.emplace_back(workerThread, std::cref(conninfo), std::ref(queue),
                             std::ref(taken), total, w);

    for (auto& t : threads) t.join();
    Logger::log("\nAll ", total, " orders processed. Check the database for results.");
    return 0;
}