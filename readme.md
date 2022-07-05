# author: daniel.deschamps@gmail.com

# build the project
$ make

# clean build outputs
$ make clean

# build and run with default arguments
$ make run

# build and run by choosing main thread to start with RR schduling and prio=10
$ make run-rr

# select the number of worker threads. default is 8 in case ommited
$ make run threads=4
$ make run-rr threads=16

# description
This program will test all 3 scheduling policies within Linux system: CFS, FIFO and RR
CFS  - Completely Fair Scheduling
FIFO - First In First Out
RR   - Round Robing

It is expected that:
- CFS  has only one priority level and it will share the CPU time among all threads which will run in parallel
- FIFO has multiple prio levels. High prio will preempt and run first. Same prios will execute in the order they started
- RR   has multiple prio levels. High prio will preempt and run first. Same prios will share time slices (100ms by default)

The program will launch a thread pool of <n> threads at the same time, with a dummy work to be done (never sleeping, yeld or blocking)
CPU affinity will be set to use only one core in order to pipe line the scheduler. You can use htop utility to check cpu usage
Threads will contain a thread id from 0 to n-1
A total of 9 sub tests will be launched by mixing scheduling policies and priority schemes
Observe that 4 priority schemes are used:
- PARALLEL   where all prios are the lowest possible and equal. (CFS is only applicable to this prio scheme)
- ASCENDING  where decreasing prios are assigned to thread id in ascending  order and theoretically low  thread id should run first
- DESCENDING where decreasing prios are assigned to thread id in descending order and theoretically high thread id should run first
- ODDS       where two prios are used. The higher prio is assigned to odd thread ids and lower prio to even thread_ids

The catch is: 
- PARALLEL RR   will run in parallel since all have same priority and cpu is shared among them.
- PARALLEL FIFO cannot be parallel as it runs one at a time since all have same prio and first in is processed until finished, then yelding to the other fellow threads
- ASCENDING/DESCENDING will behave the same for FIFO and RR since with both policies higher priority preempts lower priority
- ODDS will prioritize the odd thread ids in both FIFO and RR, however in FIFO the odds will run one at a time and with RR odds will share cpu
- CFS: has 1 prio level(0). The TimeSlice is related to the 'niceness' defined in the scheme
- FIFO has 99 prio levels.  The TimeSlice is undefined since not applicable (first in first out)
- RR   has 99 prio levels.  The TimeSlice is default 100ms and defined at system level on /proc/sys/kernel/sched_rr_timeslice_ms

# additional information

Man pages tell to compile with -pthread, but it looks that latest versions of gnu compilers have it by default

To check the number of cores
$ lscpu | egrep 'Model name|Socket|Thread|NUMA|CPU\(s\)'

Use command to monitor cpu, and verify only one core is being used.
$ htop

# CFS

The Completely Fair Scheduler, the default Linux scheduler, assigns a proportion of the processor to a process rather than a fixed timeslice. 
That means the timeslice for each process is proportional to the current load and weighted by the process' priority value.

You can tune "slice" by adjusting sched_latency_ns and sched_min_granularity_ns, but note that "slice" is not a fixed quantum. Also note that CFS preemption decisions are based upon instantaneous state. A task may have received a full (variable) "slice" of CPU time, but preemption will be triggered only if a more deserving task is available, so a "slice" is not the "max uninterrupted CPU time" that you may expect it to be.. but it is somewhat similar.

CFS has no fixed timeslice, it is calculated at runtime depending of targeted latency (sysctl_sched_latency) and number of running processes. Timeslice could never be less than minimum granularity (sysctl_sched_min_granularity).

Timeslice will be always between sysctl_sched_min_granularity and sysctl_sched_latency, which are defaults to 0.75 ms and 6 ms respectively and defined in kernel/sched/fair.c.

But actual timeslice isn't exported to user-space.

# preemption

if (policy == SCHED_FIFO || policy == SCHED_RR)
	return true;

RT policy with priority 1 runs just above SCHED_OTHER tasks.
Use 2-49 for typical app use
50 is the defaul IRQ value
51-98 for high prio threads -> real time applications
99 watchdog and migration -> System threads that must run at the highest priority.

To check if PREEMPT_RT patch is installed
$ uname -v

https://wxdublin.gitbooks.io/deep-into-linux-and-beyond/content/overview.html

The primary goal is to make the system predictable and deterministic.

It will NOT improve throughput and overall performance, instead, it will degrade overall performance.
You cannot say it ALWAYS reduce latency, often it's opposite. But the maximum latency will be reduced.
Linux RT patches are not mainstream yet. There is resistance to retain the general purpose nature of Linux.
Though they are preemptible, kernel services (such as memory allocation) do not have a guaranteed latency yet! However, not really a problem: real-time applications should allocate all their resources ahead of time.
Kernel drivers are not developed for real-time constraints yet (e.g. USB or PCI bus stacks).
Binary-only drivers have to be recompiled for RT preempt
Memory management (SLOB/SLUB allocator) still runs with preemption disabled for long periods of time
Lock around per-CPU data accesses, eliminating the preemption problem but wrecking scalability

The PREEMPT_RT patches reduce the kernel’s latency by enabling preemption in even more places than the mainline kernel currently supports: critical sections, interrupt handlers, sections which run with interrupts disabled... This helps with hard real-time workloads since there’s less chance that an uninterruptible section will delay a real-time event.

https://wxdublin.gitbooks.io/deep-into-linux-and-beyond/content/internals.html

Interrupt can be executed by a kernel thread, we can assign a priority to each thread.

One of the causes of latency involves interrupts servicing lower priority tasks. A high priority task should not be greatly affected by a low priority task, for example, doing heavy disk IO. With the normal interrupt handling in the mainline kernel, the servicing of devices like hard-drive interrupts can cause large latencies for all tasks. The RT patch uses threaded interrupt service routines to address this issue.

When a device driver requests an IRQ, a thread is created to service this interrupt line. Only one thread can be created per interrupt line. Shared interrupts are still handled by a single thread. The thread basically performs the following:

while (!kthread_should_stop()) {
  set_current_state(TASK_INTERRUPTIBLE);
  do_hardirq(desc);
  cond_resched();
  schedule();
}

# RT permission

In order to self set SCHED_RR or SCHED_FIFO will need to run the program as root user (sudo ./main) because 
$ ulimit -r
returns 0

Alternatively we could change these limits by editing /etc/security/limits.conf and add the following line
'<username> hard rtprio 99'

Then you should be able to use ulimit (from the shell) or setrlimit (from your program) to set the soft limit to the priority you need; 
alternatively, you could set that automatically by adding a second line to limits.conf, replacing hard with soft

# round robin

In order to get the default RR timeslice from system (100ms)
$ cat /proc/sys/kernel/sched_rr_timeslice_ms

# commands line tools

- Check runtime policy of program's main thread:

daniel@daniel-ubuntu:~$ ps -a
    PID TTY          TIME CMD
   2024 tty2     00:00:00 gnome-session-b
   5260 pts/1    00:32:22 htop
  30843 pts/0    00:00:00 sudo
  30845 pts/4    00:00:02 sched_test
  30862 pts/3    00:00:00 ps
daniel@daniel-ubuntu:~$ chrt -p 30845
pid 30845's current scheduling policy: SCHED_OTHER
pid 30845's current scheduling priority: 0
daniel@daniel-ubuntu:~$ chrt -p 30845
pid 30845's current scheduling policy: SCHED_RR
pid 30845's current scheduling priority: 17
daniel@daniel-ubuntu:~$ chrt -p 30845
pid 30845's current scheduling policy: SCHED_FIFO
pid 30845's current scheduling priority: 17

- start an app or change real time with a chosen policy (-o is CFS, -f is FIFO, -r is RR)

$ chrt <-o/-f-r> <prio> <app_path>
$ chrt <-o/-f-r> -p xxxx <prio> <pid>

- get the range of prios

daniel@daniel-ubuntu:~/dev/posix$ chrt -m
SCHED_OTHER min/max priority	: 0/0
SCHED_FIFO min/max priority	: 1/99
SCHED_RR min/max priority	: 1/99
SCHED_BATCH min/max priority	: 0/0
SCHED_IDLE min/max priority	: 0/0
SCHED_DEADLINE min/max priority	: 0/0

- tune shceduler parameters in kernel

Read all sched variables:

daniel@daniel-ubuntu:~/dev/posix$ sysctl -A | grep "sched"
kernel.sched_autogroup_enabled = 1
kernel.sched_cfs_bandwidth_slice_us = 5000
kernel.sched_child_runs_first = 0
kernel.sched_deadline_period_max_us = 4194304
kernel.sched_deadline_period_min_us = 100
kernel.sched_energy_aware = 1
kernel.sched_rr_timeslice_ms = 100
kernel.sched_rt_period_us = 1000000
kernel.sched_rt_runtime_us = 950000
kernel.sched_schedstats = 0
kernel.sched_util_clamp_max = 1024
kernel.sched_util_clamp_min = 1024
kernel.sched_util_clamp_min_rt_default = 1024

* sched_rt_period_us
  Period over which real-time task bandwidth enforcement is measured. The default value is 1000000 (µs).

* sched_rt_runtime_us
  Quantum allocated to real-time tasks during sched_rt_period_us. Setting to -1 disables RT bandwidth enforcement. By default, RT tasks may consume 95%CPU/sec, thus leaving 5%CPU/sec or 0.05s to be used by SCHED_OTHER tasks. The default value is 950000 (µs).

* kernel.sched_rr_timeslice_ms = 100
  Round Robin time slice

Read specific variable:

daniel@daniel-ubuntu:~/dev/posix$ sysctl kernel.sched_rr_timeslice_ms
kernel.sched_rr_timeslice_ms = 100
daniel@daniel-ubuntu:~/dev/posix$ cat /proc/sys/kernel/sched_rr_timeslice_ms
100

Set variable value:

daniel@daniel-ubuntu:~/dev/posix$ sudo sysctl kernel.sched_rr_timeslice_ms=50
kernel.sched_rr_timeslice_ms = 50
daniel@daniel-ubuntu:~/dev/posix$ cat /proc/sys/kernel/sched_rr_timeslice_ms
50

Boot time change of variables in kernel:

daniel@daniel-ubuntu:~/dev/posix$ sudo nano /etc/sysctl.conf
-> add: kernel.sched_rr_timeslice_ms = 50