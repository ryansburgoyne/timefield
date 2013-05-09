timefield-cmd : timefield-cmd.cpp Scheduler.h Task.h Scheduler.o Task.o
	g++ -g -I /usr/local/boost_1_48_0 -c timefield-cmd.cpp 
	g++ -o timefield-cmd timefield-cmd.o /usr/local/lib/libboost_date_time.a Scheduler.o Task.o
Scheduler.o : Scheduler.cpp Scheduler.h Task.h
	g++ -g -I /usr/local/boost_1_48_0 -c Scheduler.cpp
Task.o : Task.cpp Task.h
	g++ -g -I /usr/local/boost_1_48_0 -c Task.cpp	