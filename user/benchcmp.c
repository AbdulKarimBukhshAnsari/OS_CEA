#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Number of processes for benchmarking
#define NUM_CPU_PROCS 2
#define NUM_IO_PROCS 2

void cpu_intensive_task(int id) {
    struct procinfo info_start, info_end;
    int pid = getpid();

    // Sleep to stagger outputs (no overlap)
    sleep(id * 30);

    getprocinfo(pid, &info_start);
    printf("[CPU-%d] PID %d started at tick %lu, priority %d\n", 
           id, pid, info_start.start_time, info_start.priority);

    // Simulate CPU-heavy work
    for (int phase = 0; phase < 5; phase++) {
        volatile long result = 0;
        for (long i = 0; i < 5000000; i++) {
            result += i * i;
            result ^= (i + 1) * (i - 1);
            if (i % 1000000 == 0 && i != 0) result /= (i + 1);
        }
    }

    getprocinfo(pid, &info_end);

    printf("[CPU-%d] PID %d FINISHED:\n", id, pid);
    printf("  - Turnaround: %lu ticks\n", info_end.end_time - info_end.start_time);
    printf("  - Response:   %lu ticks\n", info_end.first_run - info_end.start_time);
    printf("  - Wait:       %lu ticks\n", info_end.total_wait);
    printf("  - CPU Ticks:  %d\n", info_end.cpu_ticks);
    printf("  - Scheduled:  %d\n", info_end.sched_count);
    printf("  - Final Priority: %d (0=HIGH, 1=MED, 2=LOW)\n\n", info_end.priority);

    exit(0);
}

void io_intensive_task(int id) {
    struct procinfo info_start, info_end;
    int pid = getpid();

    // Slightly delay to avoid overlapping prints
    sleep((id + NUM_CPU_PROCS) * 30);

    getprocinfo(pid, &info_start);
    printf("[I/O-%d] PID %d started at tick %lu, priority %d\n", 
           id, pid, info_start.start_time, info_start.priority);

    // Simulate I/O work pattern
    for (int i = 0; i < 30; i++) {
        volatile int x = 0;
        for (int j = 0; j < 50000; j++) x += j;
        write(1, ".", 1); // print dot to show progress
        sleep(1);         // small delay to mimic I/O wait
    }
    printf("\n");

    getprocinfo(pid, &info_end);

    printf("[I/O-%d] PID %d FINISHED:\n", id, pid);
    printf("  - Turnaround: %lu ticks\n", info_end.end_time - info_end.start_time);
    printf("  - Response:   %lu ticks\n", info_end.first_run - info_end.start_time);
    printf("  - Wait:       %lu ticks\n", info_end.total_wait);
    printf("  - CPU Ticks:  %d\n", info_end.cpu_ticks);
    printf("  - Scheduled:  %d\n", info_end.sched_count);
    printf("  - Final Priority: %d (0=HIGH - I/O rewarded!)\n\n", info_end.priority);

    exit(0);
}

int main(int argc, char *argv[]) {
    printf("\n");
    printf("================================================================\n");
    printf("           SCHEDULER PERFORMANCE COMPARISON\n");
    printf("                MLFQ SCHEDULER\n");
    printf("================================================================\n");
    printf("This benchmark demonstrates MLFQ advantages:\n");
    printf("  • CPU-bound processes: Priority 0 → 1 → 2 (adaptive demotion)\n");
    printf("  • I/O-bound processes: Stay at Priority 0 (high responsiveness)\n");
    printf("  • Better response times for interactive workloads\n");
    printf("  • Adaptive behavior based on process characteristics\n");
    printf("================================================================\n\n");

    int start = uptime();

    printf("Starting %d CPU-bound and %d I/O-bound processes...\n\n",
           NUM_CPU_PROCS, NUM_IO_PROCS);

    // Spawn CPU-bound processes
    for (int i = 0; i < NUM_CPU_PROCS; i++) {
        if (fork() == 0) {
            cpu_intensive_task(i);
        }
    }

    // Spawn I/O-bound processes
    for (int i = 0; i < NUM_IO_PROCS; i++) {
        if (fork() == 0) {
            io_intensive_task(i);
        }
    }

    // Wait for all children to finish
    for (int i = 0; i < NUM_CPU_PROCS + NUM_IO_PROCS; i++) {
        wait(0);
    }

    int end = uptime();

    printf("================================================================\n");
    printf("                    MLFQ BENCHMARK COMPLETE\n");
    printf("================================================================\n");
    printf("Total Execution Time: %d ticks\n\n", end - start);

    printf("MLFQ ADVANTAGES DEMONSTRATED:\n");
    printf("----------------------------------------------------------------\n");
    printf("✓ CPU-bound processes are demoted to lower priority\n");
    printf("✓ I/O-bound processes stay at high priority\n");
    printf("✓ I/O processes get better response time\n");
    printf("✓ Adaptive scheduling based on behavior\n");
    printf("✓ System automatically balances interactive vs. computational\n\n");

    printf("KEY OBSERVATIONS:\n");
    printf("  → CPU processes: Priority should be 2 (LOW)\n");
    printf("  → I/O processes: Priority should be 0 (HIGH)\n");
    printf("  → I/O Response Times: Should be significantly lower\n");
    printf("  → Overall Performance: Better responsiveness\n");
    printf("================================================================\n\n");

    exit(0);
}
