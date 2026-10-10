#include<iostream>
#include<vector>
#include<queue>
#include<algorithm>
#include<string>
#include<iomanip>
struct Job{
    int id;
    int arrival;
    int burst;
    int priority;
    int remaining;

    int start_time = -1;
    int finish_time = -1;
    int waiting_time = 0;
    int turnaround_time = 0;
    int response_time = -1;

};
struct Metrics {
    double avg_waiting = 0, avg_turnaround = 0, avg_response = 0, throughput = 0;
};
std::vector<Job> sampleJobs() {
    return {
        {1, 0, 6, 3, 6},
        {2, 1, 4, 1, 4},
        {3, 2, 9, 4, 9},
        {4, 3, 5, 2, 5},
        {5, 4, 2, 5, 2}
    };
}
Metrics computeMetrics(std::vector<Job>& jobs, int totalTime) {
    Metrics m;
    for (auto& j : jobs) {
        j.turnaround_time = j.finish_time - j.arrival;
        j.waiting_time = j.turnaround_time - j.burst;
        m.avg_waiting += j.waiting_time;
        m.avg_turnaround += j.turnaround_time;
        m.avg_response += j.response_time;
    }
    int n = jobs.size();
    m.avg_waiting /= n;
    m.avg_turnaround /= n;
    m.avg_response /= n;
    m.throughput = (double)n / totalTime;
    return m;
}
void printJobs(const std::string& label, std::vector<Job>& jobs, Metrics m) {
    std::cout << "\n=== " << label << " ===\n";
    std::cout << std::left << std::setw(6) << "Job" << std::setw(10) << "Arrival"
              << std::setw(8) << "Burst" << std::setw(10) << "Finish"
              << std::setw(10) << "Waiting" << std::setw(12) << "Turnaround"
              << "Response\n";
    for (auto& j : jobs) {
        std::cout << std::left << std::setw(6) << j.id << std::setw(10) << j.arrival
                   << std::setw(8) << j.burst << std::setw(10) << j.finish_time
                   << std::setw(10) << j.waiting_time << std::setw(12) << j.turnaround_time
                   << j.response_time << "\n";
    }
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Avg Waiting Time    : " << m.avg_waiting << "\n";
    std::cout << "Avg Turnaround Time : " << m.avg_turnaround << "\n";
    std::cout << "Avg Response Time   : " << m.avg_response << "\n";
    std::cout << "Throughput          : " << m.throughput << " jobs/unit-time\n";
}
Metrics runFCFS() {
    auto jobs = sampleJobs();
    std::sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        return a.arrival < b.arrival;
    });
    int clock = 0;
    for (auto& j : jobs) {
        clock = std::max(clock, j.arrival);
        j.start_time = clock;
        j.response_time = j.start_time - j.arrival;
        clock += j.burst;
        j.finish_time = clock;
    }
    auto m = computeMetrics(jobs, clock);
    printJobs("FCFS (First Come First Serve)", jobs, m);
    return m;
}
//  helper: non-preemptive scheduler (SJF & Priority) 
// returns true if job a should run before job b among ready jobs
template <typename Cmp>
Metrics runNonPreemptive(const std::string& label, Cmp better) {
    auto jobs = sampleJobs();
    int n = jobs.size(), done = 0, clock = 0;
    std::vector<bool> finished(n, false);

    while (done < n) {
        int idx = -1;
        for (int i = 0; i < n; i++) {
            if (finished[i] || jobs[i].arrival > clock) continue;
            if (idx == -1 || better(jobs[i], jobs[idx])) idx = i;
        }
        if (idx == -1) {  // CPU idle: jump to next arrival
            int next = 1e9;
            for (int i = 0; i < n; i++)
                if (!finished[i]) next = std::min(next, jobs[i].arrival);
            clock = next;
            continue;
        }
        Job& j = jobs[idx];
        j.start_time = clock;
        j.response_time = clock - j.arrival;
        clock += j.burst;
        j.finish_time = clock;
        j.remaining = 0;
        finished[idx] = true;
        done++;
    }
    auto m = computeMetrics(jobs, clock);
    printJobs(label, jobs, m);
    return m;
}

//  SJF (non-preemptive) 
Metrics runSJF() {
    return runNonPreemptive("SJF (Non-Preemptive)", [](const Job& a, const Job& b) {
        if (a.burst != b.burst) return a.burst < b.burst;
        return a.arrival < b.arrival;  // tie-break: earlier arrival
    });
}

//  Priority (non-preemptive, lower number = higher priority) 
Metrics runPriority() {
    return runNonPreemptive("Priority (Non-Preemptive)", [](const Job& a, const Job& b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        return a.arrival < b.arrival;  // tie-break: earlier arrival
    });
}

//  Round Robin
Metrics runRoundRobin(int quantum) {
    auto jobs = sampleJobs();
    std::sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        return a.arrival < b.arrival;
    });
    int n = jobs.size(), clock = 0, next = 0, done = 0;
    std::queue<int> q;

    while (done < n) {
        // admit everything that has arrived by now
        while (next < n && jobs[next].arrival <= clock) q.push(next++);

        if (q.empty()) {  // idle: jump to next arrival
            clock = jobs[next].arrival;
            continue;
        }

        int i = q.front(); q.pop();
        Job& j = jobs[i];
        if (j.start_time == -1) {
            j.start_time = clock;
            j.response_time = clock - j.arrival;
        }

        int run = std::min(quantum, j.remaining);
        clock += run;
        j.remaining -= run;

        // jobs that arrived while this one was running go in BEFORE it re-queues
        while (next < n && jobs[next].arrival <= clock) q.push(next++);

        if (j.remaining > 0) {
            q.push(i);
        } else {
            j.finish_time = clock;
            done++;
        }
    }
    std::sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        return a.id < b.id;
    });
    auto m = computeMetrics(jobs, clock);
    printJobs("Round Robin (q=" + std::to_string(quantum) + ")", jobs, m);
    return m;
}

int main() {
    Metrics fcfs = runFCFS();
    Metrics sjf  = runSJF();
    Metrics prio = runPriority();
    Metrics rr   = runRoundRobin(3);

    std::cout << "\n=== Comparison ===\n";
    std::cout << std::left << std::setw(14) << "Algorithm" << std::setw(10) << "AvgWT"
              << std::setw(10) << "AvgTAT" << std::setw(10) << "AvgRT" << "Throughput\n";
    auto row = [](const std::string& n, const Metrics& m) {
        std::cout << std::left << std::setw(14) << n << std::setw(10) << m.avg_waiting
                  << std::setw(10) << m.avg_turnaround << std::setw(10) << m.avg_response
                  << m.throughput << "\n";
    };
    row("FCFS", fcfs); row("SJF", sjf); row("Priority", prio); row("RoundRobin", rr);
    return 0;
}