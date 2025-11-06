#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// ============= NEW MLFQ HELPER FUNCTIONS - NEW CODE =============

// Get time slice for a given priority level
// Higher priority = shorter time slice for better responsiveness
int get_timeslice(int priority) {
  switch(priority) {
    case MLFQ_HIGH:   return TIMESLICE_HIGH;    // 4 ticks - shortest time slice
    case MLFQ_MEDIUM: return TIMESLICE_MEDIUM;  // 8 ticks - medium time slice
    case MLFQ_LOW:    return TIMESLICE_LOW;     // 16 ticks - longest time slice
    default:          return TIMESLICE_LOW;     // Default to lowest priority
  }
}

// Initialize MLFQ fields for a new process
// All new processes start at highest priority for best responsiveness
void init_mlfq_proc(struct proc *p) {
  p->priority = MLFQ_HIGH;                       // Start at highest priority
  p->timeslice = get_timeslice(MLFQ_HIGH);      // Set time slice for high priority
  p->timeslice_used = 0;                        // No time used yet
  p->cpu_ticks = 0;                             // No CPU time consumed
  p->sched_count = 0;                           // Not scheduled yet
  p->yielded_io = 0;                            // Not yielded for I/O
  
  // Initialize timing metrics
  p->start_time = ticks;                        // Record creation time
  p->end_time = 0;                              // Not finished yet
  p->first_run = 0;                             // Not run yet
  p->total_wait = 0;                            // No wait time yet
  p->last_scheduled = ticks;                    // Initialize last scheduled time
}

// Anti-starvation mechanism: boost all processes to highest priority
// This prevents lower-priority processes from being starved indefinitely
void
boost_all_priorities(void)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    // Don't acquire locks in interrupt context!
    // Just do the assignment - it's atomic
    if(p->state == RUNNABLE || p->state == RUNNING) {
      p->priority = MLFQ_HIGH;
      p->timeslice = get_timeslice(MLFQ_HIGH);
      p->timeslice_used = 0;
    }
  }
}
// ============= END OF NEW MLFQ HELPER FUNCTIONS =============

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid()
{
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // ============= NEW CODE: Initialize MLFQ fields =============
  init_mlfq_proc(p);  // Initialize MLFQ scheduling fields for new process
  // ============= END OF NEW CODE =============

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if(sz + n > TRAPFRAME) {
      return -1;
    }
    if((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
kfork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
kexit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;
  p->end_time = ticks;  // Record end time for performance metrics

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
kwait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if(pp->state == ZOMBIE){
          // Found one.
          pid = pp->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                  sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || killed(p)){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

/*
// ============= OLD ROUND-ROBIN SCHEDULER - COMMENTED OUT =============
// This is the original round-robin scheduler implementation
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for(;;){
    // The most recent process to run may have had interrupts
    // turned off; enable them to avoid a deadlock if all
    // processes are waiting. Then turn them back off
    // to avoid a possible race between an interrupt
    // and wfi.
    intr_on();
    intr_off();

    int found = 0;
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
        found = 1;
      }
      release(&p->lock);
    }
    if(found == 0) {
      // nothing to run; stop running on this core until an interrupt.
      asm volatile("wfi"); // wait for interrupt
    }
  }
}
// ============= END OF OLD ROUND-ROBIN SCHEDULER =============
*/

// ============= NEW MLFQ SCHEDULER IMPLEMENTATION =============
// Multi-Level Feedback Queue Scheduler
// Implements 3 priority levels with different time slices
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    intr_on();
    intr_off();

    int found = 0;
    
    for(int priority = MLFQ_HIGH; priority <= MLFQ_LOW; priority++) {
      for(p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if(p->state == RUNNABLE && p->priority == priority) {
          // Found a runnable process
          p->state = RUNNING;
          c->proc = p;
          p->sched_count++;
          
          // Track timing metrics
          if(p->first_run == 0) {
            p->first_run = ticks;  // Record first time scheduled
          }
          // Accumulate wait time since last scheduled
          p->total_wait += (ticks - p->last_scheduled);
          
          // === REMOVED: timeslice_used reset and demotion logic from here ===
          // Just do the context switch
          swtch(&c->context, &p->context);

          // Process has returned - update last scheduled time
          p->last_scheduled = ticks;
          c->proc = 0;
          found = 1;
          
          // === REMOVED: demotion check from here ===
        }
        release(&p->lock);
        if(found) break;
      }
      if(found) break;
    }
    
    if(found == 0) {
      asm volatile("wfi");
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void) // Called by processes when they want to give up the CPU (Comment added my KB)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched RUNNING");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

/*
// ============= OLD YIELD FUNCTION - COMMENTED OUT =============
// Original yield function for round-robin scheduling
// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}
// ============= END OF OLD YIELD FUNCTION =============
*/

// ============= NEW YIELD FUNCTION FOR MLFQ =============
// Modified yield function to handle I/O yielding in MLFQ
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  
  // Only set yielded_io if it's an I/O yield (not time slice expiration)
  // The mlfq_tick() function will set yielded_io = 0 for time slice expiry
  if(p->state == RUNNING) {
    p->yielded_io = 1;  // This is an I/O yield, preserve priority
  }
  
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}
// ============= END OF NEW YIELD FUNCTION =============

// ============= NEW MLFQ TIMER TICK HANDLER =============
// Timer interrupt handler - called every clock tick
// This function should be called from trap.c when a timer interrupt occurs

// Global starvation prevention counter
static int starvation_counter = 0;
#define STARVATION_THRESHOLD 10  // Boost all processes every 10 ticks

void
mlfq_tick(void)
{
  struct proc *p = myproc();
  
  // Increment global starvation counter
  starvation_counter++;
  
  if(starvation_counter >= STARVATION_THRESHOLD) {
    boost_all_priorities();
    starvation_counter = 0;
  }
  
  if(p != 0 && p->state == RUNNING) {
    // === SIMPLE ATOMIC OPERATIONS - NO LOCKS NEEDED ===
    p->cpu_ticks++;
    p->timeslice_used++;
    
    // Check if time slice is exhausted
    if(p->timeslice_used >= p->timeslice) {
      // Time slice expired - demote process
      if(p->priority < MLFQ_LOW) {
        p->priority++;
        p->timeslice = get_timeslice(p->priority);
      }
      p->yielded_io = 0;
      p->timeslice_used = 0;
      
      // === CRITICAL: Just mark for yield, don't call yield() directly ===
      // The existing yield() call in usertrap() will handle this
      p->state = RUNNABLE;
    }
  }
}
// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  extern char userret[];
  static int first = 1;
  struct proc *p = myproc();

  // Still holding p->lock from scheduler.
  release(&p->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    fsinit(ROOTDEV);

    first = 0;
    // ensure other cores see first=0.
    __sync_synchronize();

    // We can invoke kexec() now that file system is initialized.
    // Put the return value (argc) of kexec into a0.
    p->trapframe->a0 = kexec("/init", (char *[]){ "/init", 0 });
    if (p->trapframe->a0 == -1) {
      panic("exec");
    }
  }

  // return to user space, mimicing usertrap()'s return.
  prepare_return();
  uint64 satp = MAKE_SATP(p->pagetable);
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// Sleep on channel chan, releasing condition lock lk.
// Re-acquires lk when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on channel chan.
// Caller should hold the condition lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kkill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void
setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int
killed(struct proc *p)
{
  int k;
  
  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [USED]      "used",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

// ============= NEW SYSTEM CALL: getprocinfo =============
// Returns performance information for a specific process
// This system call allows user programs to get detailed performance metrics
int
getprocinfo(int pid, uint64 addr)
{
  struct proc *p;                    // Pointer to iterate through process table
  struct proc *current = myproc();   // Current calling process
  struct procinfo info;             // Structure to hold process information
  
  // Search for process with matching PID in the process table
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);               // Acquire lock to safely read process data
    
    // Check if this is the process we're looking for and it's active
    if(p->pid == pid && p->state != UNUSED) {
      // Found the process - collect performance information
      info.pid = p->pid;             // Process ID
      info.priority = p->priority;   // Current priority level (0=high, 2=low)
      info.cpu_ticks = p->cpu_ticks; // Total CPU ticks consumed by process
      info.sched_count = p->sched_count;     // Number of times process was scheduled
      info.timeslice_used = p->timeslice_used; // Ticks used in current time slice
      
      // Timing metrics for comparison
      info.start_time = p->start_time;       // When process was created
      info.end_time = p->end_time;           // When process finished (0 if running)
      info.first_run = p->first_run;         // When process was first scheduled
      info.total_wait = p->total_wait;       // Total accumulated wait time
      
      release(&p->lock);             // Release lock before copying to user space
      
      // Copy information structure to user space at provided address
      if(copyout(current->pagetable, addr, (char *)&info, sizeof(info)) < 0) {
        return -1;                   // Failed to copy to user space
      }
      
      return 0;                      // Success - information copied successfully
    }
    
    release(&p->lock);               // Release lock if this wasn't the right process
  }
  
  return -1;                         // Process not found with given PID
}
// ============= END OF NEW SYSTEM CALL =============
