#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    struct procinfo info;
    int pid = getpid();
    
    printf("=====================================\n");
    printf("     I/O-BOUND PROCESS TEST\n");
    printf("     Scheduler: MLFQ\n");
    printf("=====================================\n\n");
    
    getprocinfo(pid, &info);
    printf("Initial State:\n");
    printf("  PID: %d\n", info.pid);
    printf("  Priority: %d (MLFQ starts HIGH)\n", info.priority);
    printf("  Start Time: %lu ticks\n", info.start_time);
    printf("  CPU Ticks: %d\n", info.cpu_ticks);
    printf("  Schedule Count: %d\n\n", info.sched_count);
    
    printf("Starting I/O operations...\n");
    
    for (int i = 0; i < 30; i++) {
        // Frequent I/O operations
        for (int j = 0; j < 3; j++) {
            write(1, ".", 1);  // Quick I/O operation
        }
        
        if (i % 10 == 0) {
            printf("\n[Iteration %d] ", i);
            getprocinfo(pid, &info);
            printf("Priority: %d (MLFQ: should stay HIGH), CPU: %d, Wait: %lu, Sched: %d\n", 
                   info.priority, info.cpu_ticks, info.total_wait, info.sched_count);
        }
    }
    
    printf("\n");
    getprocinfo(pid, &info);
    printf("\n=====================================\n");
    printf("FINAL RESULTS (MLFQ):\n");
    printf("  Priority: %d (0=HIGH, 1=MED, 2=LOW)\n", info.priority);
    printf("  Turnaround Time: %lu ticks\n", info.end_time - info.start_time);
    printf("  Response Time: %lu ticks\n", info.first_run - info.start_time);
    printf("  Wait Time: %lu ticks\n", info.total_wait);
    printf("  CPU Ticks: %d\n", info.cpu_ticks);
    printf("  Schedule Count: %d\n", info.sched_count);
    printf("  Timeslice Used: %d/%d\n", info.timeslice_used,
           info.priority == 0 ? 4 : (info.priority == 1 ? 8 : 16));
    printf("\nNOTE: In MLFQ, I/O-bound stays at HIGH priority\n");
    printf("      Gets better response time than CPU-bound\n");
    printf("      Compare with RR where all are equal\n");
    printf("=====================================\n");
    
    exit(0);
}