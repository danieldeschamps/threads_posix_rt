// author: daniel.deschamps@gmail.com

// stl includes
#include <iostream>
#include <chrono>
#include <array>

// project includes
#include "threads.h"

// data and bss 
static const char * sz_line = "--------------------------------------------------------------------------------------------------------";

// code
int64_t test_wrapper(const int int_param_testnbr,
	const int int_param_policy, // from <sched.h>: SCHED_OTHER, SCHED_FIFO, SCHED_RR
	const e_processing_method e_param_method, // local enum: E_PARALLEL, E_ASCENDING, E_DESCENDING, E_ODDS
	const size_t uint_param_thread_count)
{
	std::cout << "Lauching sub test #" << int_param_testnbr << std::endl;
	auto begin = std::chrono::high_resolution_clock::now();
	create_threads(int_param_policy, e_param_method, uint_param_thread_count);
	auto end = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
	std::cout << "Finished sub test #" << int_param_testnbr << " [" << elapsed.count() * 1e-9 << " s]" << std::endl;
	std::cout << sz_line << std::endl;
	return elapsed.count();
}
// main function should be launched according to makefile directives
// make file will link it to 'sched_test' program
// root permision is required to set real time scheduling (RR and FIFO)
// by default the program's main thread is launched with CFS:
// $ sudo ./sched_test
// can launch with RR prio=10 by running:
// $ sudo chrt -rr 10 ./sched_test
int main(int argc, char *argv[]) {
	int int_threads = 8;

	if(argc > 1) {
		int_threads = std::atoi(argv[1]);
	}
	
	std::cout << sz_line << std::endl;
	std::cout << "Linux Kernel Posix Threads Real Time Scheduling Test Program. Written by daniel.deschamps@gmail.com" << std::endl;
	std::cout << "Main program started. Make sure to run with root privileges in order to be able to use RT capabilities" << std::endl;
	std::cout << "Usage: sched_test <num_threads> (default=8 if ommited)" << std::endl;
	std::cout << std::endl;
	std::cout << "This program will test 3 scheduling policies within Linux system: CFS, FIFO and RR" << std::endl;
	std::cout << "CFS  - Completely Fair Scheduling (default): shares cpu power among all threads according to niceness" << std::endl;
	std::cout << "FIFO - First In First Out (RT): Priority is enforced and cpu is shared by order" << std::endl;
	std::cout << "RR   - Round Robin (RT): Priority is enforced and cpu is shared by time slices" << std::endl;
	std::cout << std::endl;
	std::cout << "It is expected that:" << std::endl;
	std::cout << "- CFS  has only one priority level and it will share the CPU time among all threads which will run in parallel" << std::endl;
	std::cout << "- FIFO has multiple prio levels. High prio will preempt and run first. Same prios will execute in the order they started" << std::endl;
	std::cout << "- RR   has multiple prio levels. High prio will preempt and run first. Same prios will share time slices (100ms by default)" << std::endl;
	std::cout << std::endl;
	std::cout << "The program will launch a thread pool of <n> threads at the same time, with a dummy work to be done (never sleeping, yeld or blocking)" << std::endl;
	std::cout << "CPU affinity will be set to use only one core in order to pipe line the scheduler. You can use htop utility to check cpu usage" << std::endl;
	std::cout << "Threads will contain a thread id from 0 to n-1" << std::endl;
	std::cout << "A total of 9 sub tests will be launched by mixing scheduling policies and priority schemes" << std::endl;
	std::cout << "Observe that 4 priority schemes are used:" << std::endl;
	std::cout << "- PARALLEL   where all prios are the lowest possible and equal. (CFS is only applicable to this prio scheme)" << std::endl;
	std::cout << "- ASCENDING  where decreasing prios are assigned to thread id in ascending  order and theoretically low  thread id should run first" << std::endl;
	std::cout << "- DESCENDING where decreasing prios are assigned to thread id in descending order and theoretically high thread id should run first" << std::endl;
	std::cout << "- ODDS       where two prios are used. The higher prio is assigned to odd thread ids and lower prio to even thread_ids" << std::endl;
	std::cout << std::endl;
	std::cout << "The catch is: " << std::endl;
	std::cout << "- PARALLEL CFS  will run in parallel since all have same priority and cpu is shared among them (proportionally to niceness)." << std::endl;
	std::cout << "- PARALLEL RR   will run in parallel since all have same priority and cpu is shared among them (each 100ms a thread runs)." << std::endl;
	std::cout << "- PARALLEL FIFO cannot be parallel as it runs one at a time since all have same prio and first in is processed until finished, then yelding to the other fellow threads" << std::endl;
	std::cout << "- ASCENDING/DESCENDING will behave the same for FIFO and RR since with both policies higher priority preempts lower priority" << std::endl;
	std::cout << "- ODDS will prioritize the odd thread ids in both FIFO and RR, however in FIFO the odds will run one at a time and with RR odds will share cpu" << std::endl;
	std::cout << "- CFS: has 1 prio level(0). The TimeSlice is related to the 'niceness' defined in the scheme" << std::endl;
	std::cout << "- FIFO has 99 prio levels.  The TimeSlice is undefined since not applicable (first in first out)" << std::endl;
	std::cout << "- RR   has 99 prio levels.  The TimeSlice is default 100ms and defined at system level on /proc/sys/kernel/sched_rr_timeslice_ms" << std::endl;
	std::cout << "- Statistically CFS has an overall better performance than both RT policies (FIFO and RR). RT determinism comes with a small extra cost." << std::endl;
	std::cout << sz_line << std::endl;

	auto begin = std::chrono::high_resolution_clock::now();

	std::array<int64_t,9> array_testpool_ns = { 0 };
	int int_test = 0;
	array_testpool_ns[int_test++] = test_wrapper(int_test+1, SCHED_OTHER, E_PARALLEL, int_threads);
	array_testpool_ns[int_test++] = test_wrapper(int_test+1, SCHED_FIFO, E_PARALLEL, int_threads);
	array_testpool_ns[int_test++] = test_wrapper(int_test+1, SCHED_FIFO, E_ASCENDING, int_threads);
	array_testpool_ns[int_test++] = test_wrapper(int_test+1, SCHED_FIFO, E_DESCENDING, int_threads);
	array_testpool_ns[int_test++] = test_wrapper(int_test+1, SCHED_FIFO, E_ODDS, int_threads);
	array_testpool_ns[int_test++] = test_wrapper(int_test+1, SCHED_RR, E_PARALLEL, int_threads);
	array_testpool_ns[int_test++] = test_wrapper(int_test+1, SCHED_RR, E_ASCENDING, int_threads);
	array_testpool_ns[int_test++] = test_wrapper(int_test+1, SCHED_RR, E_DESCENDING, int_threads);
	array_testpool_ns[int_test++] = test_wrapper(int_test+1, SCHED_RR, E_ODDS, int_threads);

	
	auto end = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
	std::cout << "Main program finished with [" << elapsed.count() * 1E-9 << "] seconds elapsed" << std::endl;
	std::cout << std::endl;
	std::cout << "Sub tests summary:"  << std::endl;
	for (int_test=0; int_test < 9; int_test++)
		std::cout << "Sub test #" << int_test+1 << " [" << array_testpool_ns[int_test] * 1e-9 << " s] seconds elapsed" << std::endl;
	std::cout << std::endl;
	std::cout << "Statistically CFS (Sub test #1) has an overall better performance than both RT policies (FIFO and RR)." << std::endl;
	std::cout << "RT determinism comes with a small extra cost as it does not optimize scheduling fairness according to load." << std::endl;
	std::cout << sz_line << std::endl;

	return 0;
}