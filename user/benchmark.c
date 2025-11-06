#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Benchmark test that runs both CPU-bound and I/O-bound processes
// simultaneously to compare scheduler performance

void cpu_task(int id) {
    struct procinfo info;
    int pid = getpid();
    
    printf("[MLFQ-CPU-%d] Starting CPU-bound task, PID: %d\n", id, pid);
    getprocinfo(pid, &info);
    printf("[MLFQ-CPU-%d] Initial priority: %d (HIGH in MLFQ)\n", id, info.priority);
    
    for (int phase = 0; phase < 3; phase++) {
        // CPU-intensive work
        volatile long result = 0;
        for (long i = 0; i < 3000000; i++) {
            result += i * i;
            result ^= (i + 1) * (i - 1);
        }
        
        getprocinfo(pid, &info);
        printf("[MLFQ-CPU-%d] Phase %d: Priority=%d (demoted?), CPU=%d, Sched=%d\n", 
               id, phase, info.priority, info.cpu_ticks, info.sched_count);
    }
    
    getprocinfo(pid, &info);
    printf("[MLFQ-CPU-%d] DONE: Priority=%d (should be LOW), CPU=%d, Sched=%d\n",
           id, info.priority, info.cpu_ticks, info.sched_count);
    exit(0);
}

void io_task(int id) {
    struct procinfo info;
    int pid = getpid();
    
    printf("[MLFQ-I/O-%d] Starting I/O-bound task, PID: %d\n", id, pid);
    getprocinfo(pid, &info);
    printf("[MLFQ-I/O-%d] Initial priority: %d (HIGH in MLFQ)\n", id, info.priority);
    
    for (int i = 0; i < 20; i++) {
        // I/O operations
        write(1, ".", 1);
        
        if (i % 5 == 0) {
            getprocinfo(pid, &info);
            printf("\n[MLFQ-I/O-%d] Iter %d: Priority=%d (stays HIGH?), CPU=%d, Sched=%d\n", 
                   id, i, info.priority, info.cpu_ticks, info.sched_count);
        }
    }
    
    getprocinfo(pid, &info);
    printf("\n[MLFQ-I/O-%d] DONE: Priority=%d (should stay HIGH), CPU=%d, Sched=%d\n",
           id, info.priority, info.cpu_ticks, info.sched_count);
    exit(0);
}

int main(int argc, char *argv[]) {
    int num_cpu = 2;  // Number of CPU-bound processes
    int num_io = 2;   // Number of I/O-bound processes
    
    printf("=========================================\n");
    printf("    MULTI-PROCESS BENCHMARK TEST\n");
    printf("    Scheduler: MLFQ\n");
    printf("=========================================\n");
    printf("Creating %d CPU-bound and %d I/O-bound processes\n\n", num_cpu, num_io);
    
    uint64 start_time = uptime();
    
    // Fork CPU-bound processes
    for (int i = 0; i < num_cpu; i++) {
        if (fork() == 0) {
            cpu_task(i);
        }
    }
    
    // Fork I/O-bound processes
    for (int i = 0; i < num_io; i++) {
        if (fork() == 0) {
            io_task(i);
        }
    }
    
    // Wait for all children to complete
    for (int i = 0; i < num_cpu + num_io; i++) {
        wait(0);
    }
    
    uint64 end_time = uptime();
    
    printf("\n=========================================\n");
    printf("MLFQ BENCHMARK COMPLETE\n");
    printf("Total execution time: %lu ticks\n", (unsigned long)(end_time - start_time));
    printf("\nMLFQ CHARACTERISTICS:\n");
    printf("- CPU-bound drops to priority 2 (LOW)\n");
    printf("- I/O-bound stays at priority 0 (HIGH)\n");
    printf("- Better I/O responsiveness\n");
    printf("\nCompare with RR where:\n");
    printf("- All processes stay at priority 0\n");
    printf("- No workload differentiation\n");
    printf("=========================================\n");
    
    exit(0);
}
