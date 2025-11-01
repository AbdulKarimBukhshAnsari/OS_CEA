#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Pure CPU-bound test program to verify MLFQ priority demotion
int
main(int argc, char *argv[])
{
  struct procinfo info;
  int pid;
  volatile int dummy = 0;  // volatile to prevent optimization
  
  printf("=== MLFQ Priority Demotion Test ===\n");
  
  pid = getpid();
  
  // Initial priority check
  if(getprocinfo(pid, &info) == 0) {
    printf("Initial State:\n");
    printf("  Priority: %d (should be 0 - HIGH)\n", info.priority);
    printf("  CPU Ticks: %d\n", info.cpu_ticks);
    printf("  Timeslice: %d ticks\n\n", info.timeslice_used);
  }
  
  printf("Starting PURE CPU-bound workload (no I/O)...\n");
  printf("This will consume CPU continuously to trigger demotion.\n\n");
  
  // PURE CPU-bound work - NO system calls or I/O during computation!
  // This will force the process to use its full timeslice and get demoted
  for(int i = 0; i < 50000000; i++) {
    // Pure computation - no I/O, no system calls
    dummy = dummy + (i * 3) % 97;  // Some arithmetic to consume CPU
    dummy = dummy ^ (i & 0xFF);     // More operations
  }
  
  // Check priority after first burst
  if(getprocinfo(pid, &info) == 0) {
    printf("After First CPU Burst:\n");
    printf("  Priority: %d (should be 1 or 2 - DEMOTED)\n", info.priority);
    printf("  CPU Ticks: %d\n", info.cpu_ticks);
    printf("  Schedule Count: %d\n\n", info.sched_count);
  }
  
  // Another CPU burst to potentially demote further
  printf("Running second CPU burst...\n");
  for(int i = 0; i < 50000000; i++) {
    dummy = dummy + (i * 5) % 89;
    dummy = dummy ^ (i & 0xAA);
  }
  
  // Final check
  if(getprocinfo(pid, &info) == 0) {
    printf("\nFinal State:\n");
    printf("  Priority: %d (should be 2 - LOW)\n", info.priority);
    printf("  CPU Ticks: %d\n", info.cpu_ticks);
    printf("  Schedule Count: %d\n", info.sched_count);
  }
  
  printf("\n=== Test Complete ===\n");
  printf("Dummy value (to prevent optimization): %d\n", dummy);
  
  exit(0);
}