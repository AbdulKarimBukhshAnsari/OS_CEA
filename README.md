# XV6 Operating System - Complex Engineering Activity

## Project Overview

This project implements a Multi-Level Feedback Queue (MLFQ) scheduler in the xv6 operating system and develops custom applications to analyze the performance impact of different scheduling policies on I/O-bound and CPU-bound workloads.

## Learning Objectives (CLO 3)

Apply understanding of design and development principles in the construction of Operating Systems Components, specifically:
- Understanding the relationship between kernel-level scheduling decisions and user-level application behavior
- Implementing advanced scheduling algorithms in a real operating system
- Performance analysis and benchmarking of OS components

## Project Structure

```
OS_CEA/
├── xv6-riscv/              # Modified xv6 source code
│   ├── kernel/
│   │   ├── proc.c          # MLFQ scheduler implementation
│   │   ├── syscall.c       # System call implementations
│   │   └── ...
│   └── user/
│       ├── cpu_bound.c     # CPU-intensive application
│       ├── io_bound.c      # I/O-intensive application
│       └── benchmark.c     # Performance testing utility
├── docs/                   # Documentation and analysis
├── results/               # Performance test results
└── README.md             # This file
```

## Tasks Implementation

### 1. OS Initialization and Setup 
- Configure xv6 development environment with QEMU emulator
- Set up RISC-V toolchain for cross-compilation
- Study existing xv6 scheduler implementation
- Establish performance baseline with default round-robin scheduler

### 2. Kernel Modification: MLFQ Scheduler 
- **Multi-Level Feedback Queue Implementation**
  - Three priority levels (HIGH, MEDIUM, LOW)
  - Processes start at highest priority
  - Time slice-based priority demotion
  - I/O-based priority preservation
  
- **New System Call: `getprocinfo()`**
  - Returns process performance metrics
  - Tracks CPU ticks consumed
  - Counts scheduling instances

### 3. Application Development 
- **CPU-Bound Application**: Prime number calculation
- **I/O-Bound Application**: File creation and writing operations
- **Benchmark Suite**: Performance comparison tools

## Technical Specifications

### MLFQ Scheduler Design
- **Priority Levels**: 3 levels with decreasing time slices
- **Time Slices**: 
  - Level 0 (HIGH): 4 ticks
  - Level 1 (MEDIUM): 8 ticks  
  - Level 2 (LOW): 16 ticks
- **Priority Rules**:
  - New processes → HIGH priority
  - Time slice exhaustion → Demotion
  - I/O yield → Priority preservation

### System Call Interface
```c
int getprocinfo(int pid, struct procinfo* info);
```

### Performance Metrics
- CPU utilization
- Process turnaround time
- Response time
- Context switch frequency
- I/O wait time

## Development Environment

### Prerequisites
- QEMU RISC-V emulator
- RISC-V GNU toolchain
- GCC cross-compiler
- Make build system

### Setup Commands
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install qemu-system-misc gcc-riscv64-linux-gnu

# Clone and build xv6
git clone https://github.com/AbdulKarimBukhshAnsari/OS_CEA.git
cd xv6-riscv
make qemu
```

## Building and Running

### Compile xv6 with MLFQ
```bash
make clean
make qemu
```

### Run Performance Tests
```bash
# Inside xv6 shell
$ cpu_bound &
$ io_bound &
$ benchmark
```

## Performance Analysis

### Metrics Collection
- Baseline performance with round-robin scheduler
- MLFQ scheduler performance comparison
- Application-specific performance characteristics
- System throughput analysis

### Expected Outcomes
- **CPU-bound processes**: May experience slight degradation due to priority demotion
- **I/O-bound processes**: Should maintain good responsiveness
- **Interactive processes**: Improved response times
- **System fairness**: Better resource distribution

## Key Engineering Challenges

### CPA1: Abstract Thinking & Analysis
- Designing optimal time slice allocation
- Balancing fairness vs. performance
- Handling edge cases in priority management

### CPA2: Technical Integration
- Kernel-level modifications without breaking existing functionality
- User-space and kernel-space interaction design
- System call implementation and testing

### CPA3: Beyond Previous Experience
- Advanced scheduling algorithm implementation
- Real-time performance measurement
- Operating system performance optimization

## File Descriptions

| File | Purpose |
|------|---------|
| `kernel/proc.c` | MLFQ scheduler implementation |
| `kernel/syscall.c` | System call registration |
| `user/cpu_bound.c` | CPU-intensive workload generator |
| `user/io_bound.c` | I/O-intensive workload generator |
| `user/benchmark.c` | Performance measurement utility |

## Testing Strategy

1. **Unit Testing**: Individual scheduler functions
2. **Integration Testing**: Complete system functionality
3. **Performance Testing**: Comparative analysis
4. **Stress Testing**: High-load scenarios

## Documentation

- Implementation details in `/docs/implementation.md`
- Performance analysis in `/docs/performance_analysis.md`
- Testing procedures in `/docs/testing.md`

---

**Note**: This is an academic project for understanding operating system concepts. The modifications are for educational purposes and performance analysis.