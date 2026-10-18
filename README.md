# Flash-Sale-Fulfillment-Inventory-Race-Manager
//

Files: common.hpp, order_queue.hpp, main_demo.cpp (queue only), main_db.cpp (queue + PostgreSQL),
scheduler.cpp (CPU scheduling), schema.sql, reset_data.sql, demo.sql


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

## CPU Scheduling Algorithms

The CPU scheduling module simulates how incoming flash-sale orders compete for CPU processing time. Each order is represented as a process with attributes such as arrival time, burst time, and priority. The scheduler determines the order in which processes execute and helps analyze the efficiency of order processing under different workloads.

### 1. First Come First Serve (FCFS)

FCFS is a non-preemptive CPU scheduling algorithm in which processes are executed in the order they arrive in the ready queue. Once a process starts executing, it continues until completion.

**Working Principle:**
- Processes are arranged according to their arrival time.
- The process that arrives first is executed first.
- Each process runs until its burst time is completed.

**Advantages:**
- Simple to implement and understand.
- Fair according to arrival order.

**Limitation:**
- May cause the convoy effect, where short processes wait behind a long-running process.

### 2. Shortest Job First (SJF)

SJF selects the process with the shortest CPU burst time from the available processes. It can reduce average waiting time when burst times are known.

**Working Principle:**
- Identify the processes that have arrived and are ready for execution.
- Select the available process with the shortest burst time.
- Execute the selected process according to the implemented SJF variant.

**Types:**
- **Non-Preemptive SJF:** Once a process starts executing, it continues until completion.
- **Preemptive SJF (Shortest Remaining Time First):** The CPU can switch to another process if it has a shorter remaining execution time.

**Advantages:**
- Can minimize average waiting time under suitable conditions.
- Improves efficiency when short jobs are prioritized.

**Limitation:**
- Long processes may experience starvation.
- Actual burst times may be difficult to predict.

### 3. Priority Scheduling

Priority Scheduling selects processes according to their assigned priority values. It can be used to represent different urgency levels for order-processing tasks.

**Working Principle:**
- Assign a priority to each process.
- Select a ready process according to the priority rule used by the implementation.
- Execute the selected process using the configured preemptive or non-preemptive policy.

**Advantages:**
- Allows urgent tasks to receive preferential treatment.
- Supports differentiated process scheduling.

**Limitation:**
- Low-priority processes may experience starvation.
- Aging can be used to gradually increase the priority of waiting processes.

**Note:** The meaning of higher priority values depends on the implementation.

### 4. Round Robin (RR)

Round Robin is a preemptive CPU scheduling algorithm designed to share CPU time among processes. Each process receives a fixed time slice called the time quantum.

**Working Principle:**
- Processes are maintained in a ready queue.
- The scheduler selects the first ready process.
- The process executes for at most one time quantum or until completion.
- If unfinished, the process is placed back in the ready queue.
- The process continues executing in subsequent turns until completion.

**Advantages:**
- Provides regular CPU access to ready processes.
- Suitable for time-sharing systems.
- Reduces the chance of one process monopolizing the CPU.

**Limitation:**
- A very small time quantum increases context-switching overhead.
- A very large time quantum makes Round Robin behave more like FCFS.

### Scheduling Performance Metrics

The scheduling algorithms can be evaluated using the following metrics:

- **Arrival Time (AT):** Time at which a process enters the ready queue.
- **Burst Time (BT):** Total CPU execution time required by a process.
- **Completion Time (CT):** Time at which a process finishes execution.
- **Turnaround Time (TAT):** Total time from arrival to completion.

  `TAT = CT - AT`

- **Waiting Time (WT):** Total time a process spends waiting in the ready queue.

  `WT = TAT - BT`

- **Response Time (RT):** Time between process arrival and its first CPU execution.

  `RT = First CPU Start Time - AT`

- **Throughput:** Number of processes completed per unit of time.

  `Throughput = Total Completed Processes / Total Execution Time`

### Comparison of CPU Scheduling Algorithms

| Algorithm | Selection Criteria | Preemptive? | Main Characteristic |
|---|---|---|---|
| FCFS | Arrival order | No | Simple, arrival-based execution |
| SJF | Shortest burst time | Usually non-preemptive | Can reduce average waiting time |
| Priority Scheduling | Process priority | Can be either | Favors higher-priority processes |
| Round Robin | Ready-queue order and time quantum | Yes | Shares CPU time among processes |

### Relevance to Flash-Sale Fulfillment

During a flash sale, a large number of customer orders may arrive within a short period. Modeling these orders as processes allows different CPU scheduling policies to be compared under varying workloads.

The scheduling module helps analyze waiting time, turnaround time, response time, and throughput. These measurements provide insight into how scheduling decisions affect order-processing performance.

CPU scheduling is only one part of the overall system. Inventory consistency, transaction safety, synchronization, and database concurrency must also be handled by their respective modules.

### Scope and Implementation

The scheduling algorithms and metrics described above represent the project's scheduling concepts. The algorithms, variants, and measurements available in the final simulator depend on the actual implementation in `scheduler.cpp`.
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
