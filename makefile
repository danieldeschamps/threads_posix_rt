# author: daniel.deschampsgmail.com

# leading '-' ignores errors and continues
# leading '@' turns echo off

# % is the implicit rule
# $@ is the target
# $< is the first prerequisite
# $^ are all the prerequisites

CC=g++
CFLAGS=-Wall
OUT_DIR=./bin
MODULES= main threads
EXEC_FILE=sched_test

#DEPS=threads.h
OBJS=$(patsubst %,%.o,$(MODULES))
OUT_OBJS=$(patsubst %,$(OUT_DIR)/%,$(OBJS))
OUT_EXEC_FILE=$(OUT_DIR)/$(EXEC_FILE)

all: startup $(OUT_OBJS) $(OUT_EXEC_FILE)

startup:
	@-if [ ! -d "$(OUT_DIR)" ]; then mkdir $(OUT_DIR); fi

$(OUT_DIR)/%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(OUT_EXEC_FILE): $(OUT_OBJS)
	$(CC) -g $^ -o $(OUT_EXEC_FILE)

clean:
	@-if [ -d "$(OUT_DIR)" ]; then $(RM) $(OUT_DIR) -r; fi
	

pre_run: all
	@-echo --------------------------------------------------------------------------------------------------------
	@-uname -v | grep 'PREEMPT_RT'
	@-echo --------------------------------------------------------------------------------------------------------
	@-lscpu | egrep 'Model name|Socket|Thread|NUMA|CPU\(s\)'
	@-echo --------------------------------------------------------------------------------------------------------
	@-sysctl kernel.sched_rr_timeslice_ms

run: pre_run
	@-sudo $(OUT_EXEC_FILE) $(threads)

run-rr:	pre_run
	@-sudo chrt -rr 10 $(OUT_EXEC_FILE) $(threads)

-include $(DEPS)