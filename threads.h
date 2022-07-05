// author: daniel.deschamps@gmail.com

#pragma once

// linux includes
#include <sched.h> // scheduler definitions

// project defines
typedef enum e_processing_method {
    E_PARALLEL,
    E_ASCENDING,
    E_DESCENDING,
    E_ODDS
} e_processing_method;

// sub test routine. launches <n> worker threads to test each scheduling policy vs priority scheme
void create_threads(const int int_param_policy, // from <sched.h>: SCHED_OTHER, SCHED_FIFO, SCHED_RR
	const e_processing_method e_param_method, // local enum: E_PARALLEL, E_ASCENDING, E_DESCENDING, E_ODDS
	const size_t uint_param_thread_count); // number of worker threads