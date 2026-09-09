# Flash-Sale-Fulfillment-Inventory-Race-Manager

A simulation and analysis platform that models the **backend fulfillment operations of a high-concurrency flash sale**, combining **Operating Systems and Database Management Systems concepts**.
The project focuses on how thousands of concurrent orders can be scheduled, synchronized, and fulfilled while preventing **race conditions, overselling, inconsistent transactions, and deadlocks**.

> **B.Tech OS & DBMS Project — V-2026-T209**

---

##  Overview

During a flash sale, thousands of customers may attempt to purchase the same limited-stock product simultaneously. This creates a highly concurrent environment where multiple processes compete for Limited inventory, CPU processing time, Warehouse resources, Memory, Database locks, and File-system resources.
Instead of building a conventional e-commerce website, this project **simulates the backend fulfillment system** and analyzes how different OS and DBMS mechanisms behave under increasing workloads.
Orders are represented as **processes/jobs**, inventory and warehouse facilities are represented as **shared resources**, and the database maintains the persistent state of orders, inventory, payments, and shipments.

---

## Objectives

The primary objectives are to:

1. Simulate thousands of simultaneous flash-sale orders.
2. Compare different CPU/process scheduling algorithms.
3. Demonstrate multithreading and synchronization.
4. Prevent race conditions and inventory overselling.
5. Detect and prevent warehouse-resource deadlocks.
6. Simulate memory management and page replacement.
7. Demonstrate file-system operations and disk scheduling.
8. Implement transactional database operations and concurrency control.
9. Analyze system performance under different traffic conditions.
10. Demonstrate the interaction between OS resource management and DBMS consistency mechanisms.

---

## System Architecture

```text
                    ┌──────────────────────┐
                    │   Customer Requests  │
                    │   / Order Generator  │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │    Order Queue        │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │   Process Scheduler  │
                    │ FCFS | SJF | Priority│
                    │      | Round Robin   │
                    └──────────┬───────────┘
                               │
                     ┌─────────┴─────────┐
                     ▼                   ▼
              ┌──────────────┐    ┌───────────────┐
              │ Worker       │    │ Synchronization│
              │ Threads      │◄──►│ Mutex/Semaphore│
              └──────┬───────┘    │ / DB Locks     │
                     │             └───────────────┘
                     ▼
             ┌──────────────────┐
             │ Inventory        │
             │ Resource Manager │
             └────────┬─────────┘
                      │
          ┌───────────┼────────────┐
          ▼           ▼            ▼
      ┌───────┐   ┌───────┐   ┌─────────┐
      │Picking│──►│Packing│──►│Shipping │
      └───────┘   └───────┘   └─────────┘
                      │
                      ▼
             ┌──────────────────┐
             │ Relational DB    │
             │ Orders           │
             │ Inventory        │
             │ Payments         │
             │ Shipments        │
             └──────────────────┘
```

---

### Metrics

The system analyzes:
* Average waiting time
* Turnaround time
* Response time
* Throughput
* CPU utilization
* Inventory utilization
* Page-fault rate
* Disk head movement
* Deadlock occurrences
* Transaction success/failure rate
* Database performance
* Resource utilization

---

### Operating Systems
* Process Scheduling
* Threads
* Synchronization
* Deadlocks
* Memory Management
* File Systems
* Disk Scheduling

### Database Systems
* Transactions
* ACID
* Concurrency Control
* Locking
* Isolation
* Recovery
* Indexing
* Normalization

---

## Proposed Technology Stack

| Layer                    | Technology                         |
| ------------------------ | ---------------------------------- |
| Core Simulation          | C / C++                            |
| Multithreading           | POSIX Threads / C++ Threads        |
| Database                 | PostgreSQL                         |
| Database Connectivity    | PostgreSQL Client Library / Driver |
| Frontend / Visualization | HTML, CSS, JavaScript              |
| Charts                   | Chart.js                           |
| Version Control          | Git + GitHub                       |

> The core OS simulations will be implemented in C/C++, while PostgreSQL will handle the relational database component.

---

The final system will demonstrate how **process scheduling, synchronization, resource allocation, memory management, file systems, transactions, and database concurrency control work together in a high-concurrency fulfillment environment.**

---

## References

1. Abraham Silberschatz, Peter B. Galvin, Greg Gagne, **Operating System Concepts**, 10th Edition, Wiley.
2. Abraham Silberschatz, Henry F. Korth, S. Sudarshan, **Database System Concepts**, 7th Edition, McGraw-Hill.
3. [PostgreSQL 18 Documentation](https://www.postgresql.org/docs)
4. [Linux Kernel Documentation](https://docs.kernel.org)

---

## Roles Information

**Samridhi Gupta:** Concurrency, synchronization module
**Sejal Kaur:** System design, scheduling
algorithms
**Palak Sajwan:** Memory/disk simulation,
documentation
**Pushkar Jain:** Database schema, transactions, evaluation

---
