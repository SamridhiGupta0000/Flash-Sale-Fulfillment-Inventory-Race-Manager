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
