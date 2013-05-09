/* 
 * Scheduler.h
 * Ryan Burgoyne
 * 05 Dec 2011
 * TimeField Scheduler
 * This file provides the definitions for the Scheduler class which manages the
 * tasks for TimeField.
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <string>

#include "Task.h"

class Scheduler {
private:
    boost::posix_time::time_period *workingInterval;
    std::vector<Task *> taskList;
    std::string tasksFilename;

public:
    Scheduler(std::string tasksFilename);
    ~Scheduler();
    boost::posix_time::time_period *getWorkingInterval();    
    void setWorkingInterval(boost::posix_time::time_period *);
    void addTask(Task *task);
    void deleteTask(int i);
    Task *getTask(int i);
    std::vector<Task *> getTaskList();
};

#endif