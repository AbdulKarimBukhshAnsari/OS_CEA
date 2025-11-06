#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Pure CPU-intensive function
void cpu_intensive_work(int iterations) {
    volatile long result = 0;
    // Heavy computation without any I/O
    for (long i = 0; i < iterations; i++) {
        result += i * i;                    // Square calculation
        result ^= (i + 1) * (i - 1);        // More arithmetic
        result = (result << 3) | (result >> 5);  // Bit manipulation
    }
    // Use result to prevent optimization
    if (result == 0x12345678) {
        printf("impossible\n");
    }
}

int main(int argc, char *argv[]) {
    struct procinfo info;
    int pid = getpid();
    
    printf("=====================================\n");
    printf("     CPU-BOUND PROCESS TEST\n");
    printf("     Scheduler: MLFQ\n");
    printf("=====================================\n\n");
    
    // Initial state
    getprocinfo(pid, &info);
    printf("Initial State:\n");
    printf("  PID: %d\n", info.pid);
    printf("  Priority: %d (MLFQ starts HIGH)\n", info.priority);
    printf("  Start Time: %lu ticks\n", info.start_time);
    printf("  CPU Ticks: %d\n", info.cpu_ticks);
    printf("  Schedule Count: %d\n\n", info.sched_count);
    
    // PHASE 1: First CPU burst (should demote to priority 1)
    printf("PHASE 1 - Starting first CPU burst...\n");
    cpu_intensive_work(5000000);
    
    getprocinfo(pid, &info);
    printf("  After Phase 1:\n");
    printf("    Priority: %d (MLFQ: demoted?)\n", info.priority);
    printf("    CPU Ticks: %d\n", info.cpu_ticks);
    printf("    Wait Time: %lu ticks\n", info.total_wait);
    printf("    Schedule Count: %d\n\n", info.sched_count);
    
    // PHASE 2: Second CPU burst (should demote to priority 2)
    printf("PHASE 2 - Starting second CPU burst...\n");
    cpu_intensive_work(8000000);
    
    getprocinfo(pid, &info);
    printf("  After Phase 2:\n");
    printf("    Priority: %d (MLFQ: should be LOW)\n", info.priority);
    printf("    CPU Ticks: %d\n", info.cpu_ticks);
    printf("    Wait Time: %lu ticks\n", info.total_wait);
    printf("    Schedule Count: %d\n\n", info.sched_count);
    
    // PHASE 3: Final CPU burst (should remain at priority 2)
    printf("PHASE 3 - Starting final CPU burst...\n");
    cpu_intensive_work(10000000);
    
    // Final state
    getprocinfo(pid, &info);
    printf("=====================================\n");
    printf("FINAL RESULTS (MLFQ):\n");
    printf("  Priority: %d (0=HIGH, 1=MED, 2=LOW)\n", info.priority);
    printf("  Turnaround Time: %lu ticks\n", info.end_time - info.start_time);
    printf("  Response Time: %lu ticks\n", info.first_run - info.start_time);
    printf("  Wait Time: %lu ticks\n", info.total_wait);
    printf("  CPU Ticks: %d\n", info.cpu_ticks);
    printf("  Schedule Count: %d\n", info.sched_count);
    printf("  Timeslice Used: %d/%d\n", info.timeslice_used, 
           info.priority == 0 ? 4 : (info.priority == 1 ? 8 : 16));
    printf("\nNOTE: In MLFQ, CPU-bound drops to priority 2\n");
    printf("      Compare with RR where it stays at 0\n");
    printf("=====================================\n");
    
    exit(0);
}