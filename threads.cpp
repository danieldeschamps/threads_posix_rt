// author: daniel.deschamps@gmail.com

// stl includes
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>

// linux includes
#include <pthread.h> // posix threads interface
#include <unistd.h> // sleep
#include <string.h> // strerror - converts sys errors to string

// project includes
#include "threads.h"

// project defines
#define cout_sys_error(ret,func) if (ret != 0) { std::cout << "* syscall error * " << func << " ret=[" << int_ret<< "] [" << strerror(int_ret) <<"]" << std::endl; exit(EXIT_FAILURE);}// sys error console output

typedef struct thread_information_t {
	pthread_t   thread_handle;
	int         int_id;
	std::string	str_name;
	int			int_retval;
} thread_information_t; // thread argument to be passed from main thread

// data and bss
static const char *arr_policies[] = {"CFS","FIFO","RR"}; // convert policy to text
static thread_information_t *array_threads; // Thread pool information
static pthread_mutex_t mutex_cout; // mutex to protect concurrent std::couts (avoid chars scrambling at console)

// code

// helper function returns a formatted string containing calling thread information
static std::string get_sched_info_str() {
	int int_ret, int_policy, int_prio, int_minprio, int_maxprio;
	unsigned int uint_timeslice_ms;
	pthread_t o_thread_handle;
	sched_param o_sched_param;
	timespec o_timespec;
	std::string str_return;

	o_thread_handle = pthread_self();

	int_ret = pthread_getschedparam(o_thread_handle, &int_policy, &o_sched_param);
	cout_sys_error(int_ret,"pthread_getschedparam");

	int_minprio = sched_get_priority_min(int_policy);
	int_maxprio = sched_get_priority_max(int_policy);

	int_ret = sched_rr_get_interval(0, &o_timespec); // 0 is the calling process
	cout_sys_error(int_ret,"sched_rr_get_interval");
	
	uint_timeslice_ms = static_cast<unsigned int> (o_timespec.tv_nsec / 1E6); // explicit cast for maintainability

	str_return +=  
		"Thread Handle=["	+ std::to_string(o_thread_handle) 				+ "] "
		"Policy=["			+ arr_policies[int_policy] 						+ "] "
		"MinPrio=["			+ std::to_string(int_minprio) 					+ "] "
		"MaxPrio=["			+ std::to_string(int_maxprio)					+ "] "
		"TmSlice=["			+ std::to_string(uint_timeslice_ms) 			+ " ms] "
		"SetPrio=["			+ std::to_string(o_sched_param.sched_priority) 	+ "] ";

	return str_return;
}

// standard worker thread routine. should receive a thread_information_t* as argument
static void* thread_routine(void* arg) {
	int int_ret;
	thread_information_t * po_thread = reinterpret_cast<thread_information_t*>(arg);
	std::ostringstream oss_threadinfo_custom, oss_threadinfo_sys, oss_threadbenchmark, oss_start, oss_end;
	timespec o_timespec;
	
	auto begin = std::chrono::high_resolution_clock::now();

	oss_threadinfo_custom << "id=[" << po_thread->int_id << "] name=[" << po_thread->str_name << "] ";
	oss_threadinfo_sys  << get_sched_info_str();

	oss_start << "->Work Thread: " << oss_threadinfo_custom.str() << oss_threadinfo_sys.str() << std::endl;

	pthread_mutex_lock(&mutex_cout);
	std::cout << oss_start.str(); // protected console output
	pthread_mutex_unlock(&mutex_cout);

	for (size_t i = 0; i < 0x3FFFFFFF; i++)
	{
		// dummy work
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
	
	oss_threadbenchmark << "Elapsed=[" << elapsed.count() * 1e-9 << " s]";

	oss_end << "<-End  Thread: " << oss_threadinfo_custom.str() << oss_threadbenchmark.str() << std::endl;

	pthread_mutex_lock(&mutex_cout);
	std::cout << oss_end.str(); // protected console output
	pthread_mutex_unlock(&mutex_cout);

	po_thread->int_retval = -po_thread->int_id;
	return reinterpret_cast<void*>(&(po_thread->int_retval));
}

// sub test routine. launches <n> worker threads to test each scheduling policy vs priority scheme
void create_threads(const int int_param_policy, // from <sched.h>: SCHED_OTHER, SCHED_FIFO, SCHED_RR
	const e_processing_method e_param_method, // local enum: E_PARALLEL, E_ASCENDING, E_DESCENDING, E_ODDS
	const size_t uint_param_thread_count) // number of worker threads
{
// variables
	int int_ret = 0;

// initialize global thread information
	array_threads = new thread_information_t [uint_param_thread_count] ;
	for(size_t i = 0; i < uint_param_thread_count; i++) {
		array_threads[i].int_id = i;
		array_threads[i].str_name = "Thread " + std::to_string(i);
	}

// initialize semaphores -> must call destroy later: pthread_mutex_destroy
	pthread_mutex_init(&mutex_cout, NULL);

// program it to use only one core in order to check scheduler behavior
	cpu_set_t o_cpu_set;
	CPU_ZERO(&o_cpu_set);        // clear cpu mask
	CPU_SET(0, &o_cpu_set);      // set cpu 0
	sched_setaffinity(0, sizeof(cpu_set_t), &o_cpu_set);  // 0 is the calling process

// display main thread attibutes according to command line launch
	// by default system launches with CFS
	// use 'sudo chrt -rr 10 ./sched_test' in order to launch with RR prio=10
	std::cout << "Main Thread: " << get_sched_info_str() << std::endl;

// handle thread attributes
	pthread_attr_t o_pthread_attr;
	sched_param o_sched_param; // contains only the thread priority
	int int_policy = -1, int_minprio = -1, int_maxprio = -1;

	// initialized with default attributes to be used with pthread_create. 
	// these attributes should be the same as calling pthread_create with attributes pointer
	// need to call pthread_attr_destroy after use
	int_ret = pthread_attr_init (&o_pthread_attr);  
 	cout_sys_error(int_ret,"pthread_attr_init");

	// this is needed in order to make pthread_create to accept scheduling parameters within pthread_attr_t
	// affects: 1-pthread_attr_setschedpolicy, 2-pthread_attr_setschedparam, 3-pthread_attr_setscope
	// PTHREAD_INHERIT_SCHED = inherit parameters from calling thread (default)
	// PTHREAD_EXPLICIT_SCHED = use pthread_attr_t parameter passed to pthread_create
	 int_ret = pthread_attr_setinheritsched(&o_pthread_attr,PTHREAD_EXPLICIT_SCHED); 
	cout_sys_error(int_ret,"pthread_attr_setinheritsched");

	// set scheduling algorithm. requires pthread_attr_setinheritsched(&o_pthread_attr,PTHREAD_EXPLICIT_SCHED);
	int_ret = pthread_attr_setschedpolicy(&o_pthread_attr, int_param_policy); 
	cout_sys_error(int_ret,"pthread_attr_setschedpolicy");

	int_maxprio = sched_get_priority_max(int_param_policy);
	int_minprio = sched_get_priority_min(int_param_policy);
	
// create threads
	// but first set calling thread to the same policy in order to be in the priority pool
	o_sched_param.sched_priority = int_minprio;
	if(int_param_policy > 0) { // only for FIFO or RR
		// this avoids the launched threads from starving the main thread before it launches all thread pool in preemptable policies (FIFO and RR)
		o_sched_param.sched_priority = int_minprio+uint_param_thread_count;
	}
	int_ret = pthread_setschedparam(pthread_self(), int_param_policy, &o_sched_param); // set sched param (just policy and prio) directly to the running thread
	cout_sys_error(int_ret,"pthread_setschedparam");

	// print updated main thread sched.
	std::cout << "Main Thread: " << get_sched_info_str() << std::endl;

	for(size_t i = 0; i < uint_param_thread_count; i++) {
		// set the priority according to the allowed in the scheduling policy selected, otherwise setschedparam will fail with (invalid parameter error - 22)

		o_sched_param.sched_priority = int_minprio; // all in parallel
		if(int_param_policy > 0) { // only for FIFO or RR
			switch (e_param_method) {
				case E_PARALLEL:
					o_sched_param.sched_priority = int_minprio; // all in parallel
				break;
				case E_ASCENDING:
					o_sched_param.sched_priority = int_minprio+uint_param_thread_count-1-i; // One at a time, with ascending order
				break;
				case E_DESCENDING:
					o_sched_param.sched_priority = int_minprio+i; // One at a time, with descending order
				break;
				case E_ODDS:
					o_sched_param.sched_priority = int_minprio+(i%2); // odds will finish first
				break;
			}
		}

		// at this point the o_pthread_attr already contains (1. default init) (2. PTHREAD_EXPLICIT_SCHED flag) (3. sched policy)
		// now we will add the (4. priority) to it via o_sched_param
		int_ret = pthread_attr_setschedparam(&o_pthread_attr,&o_sched_param);
		cout_sys_error(int_ret,"pthread_attr_setschedparam");

		// now thread will be created with o_pthread_attr containing all (4 configurations) required to change scheduling properties
		int_ret = pthread_create(&array_threads[i].thread_handle, &o_pthread_attr, &thread_routine, (void*) & array_threads[i]); // o_pthread_attr as NULL would initialize threads with default parameters
		cout_sys_error(int_ret,"pthread_create");
	}

	const char *arr_methods[] = {"PARALLEL","ASCENDING","DESCENDING", "ODDS"}; // priority schemes translated to text

	pthread_mutex_lock(&mutex_cout);  // after we have parallel threads running, we need protected console output
	std::cout << "Main Thread: Finished Launching All Worker Threads. Policy=[" << arr_policies[int_param_policy] << "] Method=[" << arr_methods[e_param_method] << "]" << std::endl;
	pthread_mutex_unlock(&mutex_cout);


// try to change priorities after launch - disabled for now
	for(size_t i = 0; i < uint_param_thread_count; i++) {
		// int_ret = pthread_setschedprio(array_threads[i].thread_handle, int_minprio+(int_maxprio/2)+i);
	}

// join threads
	for(size_t i = 0; i < uint_param_thread_count; i++) {
		void* p_return = &array_threads[i].int_retval;
		int_ret = pthread_join(array_threads[i].thread_handle, &p_return);
		cout_sys_error(int_ret,"pthread_join");
	}

	for(size_t i = 0; i < uint_param_thread_count; i++) {
		std::cout << "Main Thread joined with: Work Thread id=[" << array_threads[i].int_id << "]" << " Retval=[" << array_threads[i].int_retval << "]"   << std::endl;
	}

// destroy initialized api objects
	int_ret = pthread_mutex_destroy(&mutex_cout);
	cout_sys_error(int_ret,"pthread_mutex_destroy");

	int_ret = pthread_attr_destroy (&o_pthread_attr); // destroy attributes.
	cout_sys_error(int_ret,"pthread_attr_destroy");

// clean the heap
	delete [] array_threads;
}