/* 
 * Task.cpp
 * Ryan Burgoyne
 * 06 Dec 2011
 * TimeField Task
 * This file provides the implementation for the Task class for TimeField.
 */

#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>
#include <vector>

#include "Task.h"

Task::Task(std::string title, std::string notes, 
           boost::posix_time::time_period *interval,
           boost::posix_time::time_duration *duration,
           Task *parent) {
    Task::title = title;
    Task::notes = notes;
    Task::interval = interval;
    Task::duration = duration;
    Task::parent = parent;    
}

Task::~Task() {
    delete interval;
    delete duration;
}