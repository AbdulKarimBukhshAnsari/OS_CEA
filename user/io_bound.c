#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    struct procinfo info;
    int pid = getpid();
    
    printf("=== Better I/O-Bound Test ===\n");
    getprocinfo(pid, &info);
    printf("Start: Priority %d\n\n", info.priority);
    
    for (int i = 0; i < 40; i++) {
        // MORE FREQUENT I/O with less computation
        for (int j = 0; j < 3; j++) {
            write(1, ".", 1);  // Quick I/O operation
        }
        
        printf(" I/O %d: ", i);
        getprocinfo(pid, &info);
        printf("Priority %d, CPU Ticks: %d\n", info.priority, info.cpu_ticks);
        
        // More frequent checks
        if (i % 3 == 0) {
            getprocinfo(pid, &info);
            printf("  [Quick Check] Priority: %d\n", info.priority);
        }
    }
    
    getprocinfo(pid, &info);
    printf("\nFinal: Priority %d, CPU Ticks: %d\n", info.priority, info.cpu_ticks);
    
    exit(0);
}