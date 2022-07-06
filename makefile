# author: daniel.deschampsgmail.com

all: threads main link

threads: threads.cpp
	g++ -c threads.cpp -o ./bin/threads.o

main: main.cpp
	g++ -c main.cpp -o ./bin/main.o

link: ./bin/threads.o ./bin/main.o
	g++ -g ./bin/threads.o ./bin/main.o -o ./bin/sched_test

clean:
# leading '-' ignores errors and continues
	-rm ./bin/*.o
	-rm ./bin/sched_test

pre_run: all
# @ turns echo off
	@cp ./bin/sched_test ./
	@-chmod a+rwx ./sched_test
	@-chmod a+rwx ./bin
	@-chmod a+rwx ./
	@-echo --------------------------------------------------------------------------------------------------------
	@-uname -v | grep 'PREEMPT_RT'
	@-lscpu | egrep 'Model name|Socket|Thread|NUMA|CPU\(s\)'

run: pre_run
	@-sudo ./sched_test $(threads)
	@rm ./sched_test

run-rr: pre_run
	@-sudo chrt -rr 10 ./sched_test $(threads)
	@rm ./sched_test