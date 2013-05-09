/* 
 * Task.h
 * Ryan Burgoyne
 * 06 Dec 2011
 * TimeField Task
 * This file provides the definitions for the Task class for TimeField.
 */

#ifndef TASK_H
#define TASK_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <string>
#include <vector>

class Task {
private:
    std::string title;
    std::string notes;
    boost::posix_time::time_period *interval;
    boost::posix_time::time_duration *duration;
    Task *parent;
    std::vector<Task *> children;
    
public:
    Task(std::string title, std::string notes, 
         boost::posix_time::time_period *interval,
         boost::posix_time::time_duration *duration,
         Task *parent);
    ~Task();
    std::string getTitle() { return title; }
    std::string getNotes() { return notes; }
    boost::posix_time::time_period *getInterval() { return interval; }
    boost::posix_time::time_duration *getDuration() { return duration; }
};

#endif