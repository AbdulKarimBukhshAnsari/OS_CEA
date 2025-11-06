#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Enhanced benchmark that shows clear metrics for MLFQ vs RR comparison
// This demonstrates MLFQ's advantages in:
// 1. Response time for I/O-bound processes
// 2. Overall turnaround time
// 3. Fair CPU distribution

#define NUM_CPU_PROCS 2
#define NUM_IO_PROCS 2

void cpu_intensive_task(int id) {
    struct procinfo info_start, info_end;
    int pid = getpid();
    
    // Get initial stats
    getprocinfo(pid, &info_start);
    
    printf("[CPU-%d] PID %d starting at tick %lu, priority %d\n", 
           id, pid, info_start.start_time, info_start.priority);
    
    // CPU-intensive work - longer computation
    for (int phase = 0; phase < 5; phase++) {
        volatile long result = 0;
        for (long i = 0; i < 5000000; i++) {
            result += i * i;
            result ^= (i + 1) * (i - 1);
            if (i % 1000000 == 0) {
                result = result / (i + 1);  // Add more CPU work
            }
        }
    }
    
    // Get final stats
    getprocinfo(pid, &info_end);
    
    printf("[CPU-%d] PID %d FINISHED:\n", id, pid);
    printf("  - Turnaround Time: %lu ticks (start=%lu, end=%lu)\n", 
           info_end.end_time - info_end.start_time,
           info_end.start_time, info_end.end_time);
    printf("  - Response Time:   %lu ticks (first scheduled at %lu)\n", 
           info_end.first_run - info_end.start_time, info_end.first_run);
    printf("  - Wait Time:       %lu ticks\n", info_end.total_wait);
    printf("  - CPU Ticks:       %d\n", info_end.cpu_ticks);
    printf("  - Scheduled:       %d times\n", info_end.sched_count);
    printf("  - Final Priority:  %d%s\n", 
           info_end.priority,
           info_end.priority > 0 ? " (DEMOTED by MLFQ)" : " (HIGH)");
    
    exit(0);
}

void io_intensive_task(int id) {
    struct procinfo info_start, info_end;
    int pid = getpid();
    
    // Get initial stats
    getprocinfo(pid, &info_start);
    
    printf("[I/O-%d] PID %d starting at tick %lu, priority %d\n", 
           id, pid, info_start.start_time, info_start.priority);
    
    // I/O-intensive work - lots of short bursts with I/O
    for (int i = 0; i < 30; i++) {
        // Small CPU burst
        volatile int x = 0;
        for (int j = 0; j < 50000; j++) {
            x += j;
        }
        
        // I/O operation
        write(1, ".", 1);
        
        // Another small burst
        for (int j = 0; j < 50000; j++) {
            x += j;
        }
    }
    
    printf("\n");
    
    // Get final stats
    getprocinfo(pid, &info_end);
    
    printf("[I/O-%d] PID %d FINISHED:\n", id, pid);
    printf("  - Turnaround Time: %lu ticks (start=%lu, end=%lu)\n", 
           info_end.end_time - info_end.start_time,
           info_end.start_time, info_end.end_time);
    printf("  - Response Time:   %lu ticks (first scheduled at %lu)\n", 
           info_end.first_run - info_end.start_time, info_end.first_run);
    printf("  - Wait Time:       %lu ticks\n", info_end.total_wait);
    printf("  - CPU Ticks:       %d\n", info_end.cpu_ticks);
    printf("  - Scheduled:       %d times\n", info_end.sched_count);
    printf("  - Final Priority:  %d%s\n", 
           info_end.priority,
           info_end.priority == 0 ? " (HIGH - I/O rewarded)" : " (lower)");
    
    exit(0);
}

int main(int argc, char *argv[]) {
    printf("\n");
    printf("================================================================\n");
    printf("           SCHEDULER PERFORMANCE COMPARISON\n");
    printf("================================================================\n");
    printf("This benchmark measures key performance metrics:\n");
    printf("  • Turnaround Time: Total time from creation to completion\n");
    printf("  • Response Time:   Time until first CPU allocation\n");
    printf("  • Wait Time:       Total time spent waiting for CPU\n");
    printf("  • Priority:        Final priority (shows MLFQ adaptation)\n");
    printf("================================================================\n\n");
    
    uint64 benchmark_start = uptime();
    
    printf("Starting %d CPU-bound and %d I/O-bound processes...\n\n", 
           NUM_CPU_PROCS, NUM_IO_PROCS);
    
    // Fork CPU-bound processes
    for (int i = 0; i < NUM_CPU_PROCS; i++) {
        int pid = fork();
        if (pid == 0) {
            cpu_intensive_task(i);
        }
    }
    
    // Fork I/O-bound processes
    for (int i = 0; i < NUM_IO_PROCS; i++) {
        int pid = fork();
        if (pid == 0) {
            io_intensive_task(i);
        }
    }
    
    // Wait for all children
    for (int i = 0; i < NUM_CPU_PROCS + NUM_IO_PROCS; i++) {
        wait(0);
    }
    
    uint64 benchmark_end = uptime();
    uint64 total_time = benchmark_end - benchmark_start;
    
    printf("\n================================================================\n");
    printf("                    BENCHMARK COMPLETE\n");
    printf("================================================================\n");
    printf("Total Execution Time: %lu ticks\n\n", total_time);
    
    printf("KEY OBSERVATIONS:\n");
    printf("----------------------------------------------------------------\n");
    printf("In MLFQ:\n");
    printf("  ✓ CPU-bound processes are demoted to lower priority\n");
    printf("  ✓ I/O-bound processes stay at high priority\n");
    printf("  ✓ I/O processes get better response time\n");
    printf("  ✓ Better interactive responsiveness\n");
    printf("  ✓ Adaptive scheduling based on behavior\n\n");
    
    printf("In Round-Robin:\n");
    printf("  • All processes stay at priority 0\n");
    printf("  • No differentiation between CPU/I/O bound\n");
    printf("  • Equal treatment regardless of behavior\n");
    printf("  • I/O processes may wait longer\n");
    printf("  • Longer response times for interactive tasks\n\n");
    
    printf("EXPECTED MLFQ ADVANTAGES:\n");
    printf("  → Lower response time for I/O processes\n");
    printf("  → Better overall turnaround time\n");
    printf("  → More context switches (better interactivity)\n");
    printf("  → Visible priority adaptation\n");
    printf("================================================================\n\n");
    
    exit(0);
}
