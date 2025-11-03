#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int cpu_pid, io_pid;
    
    printf("=== MLFQ Benchmark Launcher ===\n");
    printf("Starting CPU-bound and I/O-bound processes simultaneously...\n\n");
    
    // Start CPU-bound process
    cpu_pid = fork();
    if (cpu_pid == 0) {
        // Child process - run CPU-bound
        char *args[] = {"cpu_bound", 0};
        exec("cpu_bound", args);
        exit(0);
    }
    
    // Start I/O-bound process  
    io_pid = fork();
    if (io_pid == 0) {
        // Child process - run I/O-bound
        char *args[] = {"io_bound", 0};
        exec("io_bound", args);
        exit(0);
    }
    
    printf("CPU-bound PID: %d\n", cpu_pid);
    printf("I/O-bound PID: %d\n", io_pid);
    printf("\nBoth processes running...\n");
    
    // Wait for both to complete
    wait(0);
    wait(0);
    
    printf("\n=== Benchmark Complete ===\n");
    
    exit(0);
}