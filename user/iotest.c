#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    struct procinfo info;
    int pid = getpid();
    
    printf("=== Pure I/O Test ===\n");
    getprocinfo(pid, &info);
    printf("Start: Priority %d\n\n", info.priority);
    
    for(int i = 0; i < 10000; i++) {
        // PURE I/O - no computation between
        write(1, "X", 1);
        write(1, "Y", 1);
        write(1, "Z", 1);
        
        if(i % 5 == 0) {
            printf("Check %d: ", i);
            getprocinfo(pid, &info);
            printf("Priority %d\n", info.priority);
        }
    }
    
    getprocinfo(pid, &info);
    printf("\nFinal: Priority %d\n", info.priority);
    
    exit(0);
}