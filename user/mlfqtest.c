#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Test program to verify MLFQ scheduler and getprocinfo system call
int
main(int argc, char *argv[])
{
  struct procinfo info;
  int pid, i;
  
  printf("=== MLFQ Scheduler Test ===\n");
  
  // Test getprocinfo system call
  pid = getpid();
  if(getprocinfo(pid, &info) == 0) {
    printf("Process %d Info:\n", pid);
    printf("  Priority: %d\n", info.priority);
    printf("  CPU Ticks: %d\n", info.cpu_ticks);
    printf("  Schedule Count: %d\n", info.sched_count);
    printf("  Time Slice Used: %d\n", info.timeslice_used);
  } else {
    printf("getprocinfo failed for PID %d\n", pid);
  }
  
  // CPU-bound workload to test priority demotion
  printf("\nStarting CPU-bound workload...\n");
  for(i = 0; i < 1000000; i++) {
    // Busy work to consume CPU time
    if(i % 100000 == 0) {
      if(getprocinfo(pid, &info) == 0) {
        printf("After %d iterations - Priority: %d, CPU Ticks: %d\n", 
               i, info.priority, info.cpu_ticks);
      }
    }
  }
  
  // Final process info
  if(getprocinfo(pid, &info) == 0) {
    printf("\nFinal Process Info:\n");
    printf("  Priority: %d\n", info.priority);
    printf("  CPU Ticks: %d\n", info.cpu_ticks);
    printf("  Schedule Count: %d\n", info.sched_count);
  }
  
  printf("=== Test Complete ===\n");
  exit(0);
}