/* 
 * Scheduler.cpp
 * Ryan Burgoyne
 * 05 Dec 2011
 * TimeField Scheduler
 * This file provides the implementation for the Scheduler class which manages
 * the tasks for TimeField.
 */

#include <string>
#include <vector>
#include <exception>
#include <boost/date_time/posix_time/posix_time.hpp>

// for loading tasks.xml
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include "Scheduler.h"

Scheduler::Scheduler(std::string tasksFilename) {
    Scheduler::tasksFilename = tasksFilename;
    // Set working interval to the current day
    workingInterval = new boost::posix_time::
    time_period(boost::posix_time::ptime(boost::gregorian::day_clock::
                                         local_day()),
                boost::posix_time::hours(24));
    
    // Read persistent task data from file
    try {
        boost::property_tree::ptree pt;
        read_xml(tasksFilename, pt);
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
                      pt.get_child("tasks"))
        {
            std::string title, notes, releaseDateString, dueDateString, 
            durationString;
            title = v.second.get<std::string>("title");        
            notes = v.second.get<std::string>("notes");
            releaseDateString = v.second.get<std::string>("release-date");        
            dueDateString = v.second.get<std::string>("due-date");        
            durationString = v.second.get<std::string>("duration");
            
            boost::posix_time::ptime releaseDate =
            boost::posix_time::time_from_string(releaseDateString);
            boost::posix_time::ptime dueDate =
            boost::posix_time::time_from_string(dueDateString);
            boost::posix_time::time_period *interval = 
            new boost::posix_time::time_period(releaseDate, dueDate);
            
            boost::posix_time::time_duration *duration =
            new boost::posix_time::
            time_duration(boost::posix_time::
                          duration_from_string(durationString));
            addTask(new Task(title, notes, interval, duration, NULL));
        }
    }
    catch (...) {
        // Failed to read from file.
        taskList.clear();
    }
}

Scheduler::~Scheduler() {
    // Write persistent task data to file
    boost::property_tree::ptree pt;
    BOOST_FOREACH(Task *task, taskList)
    {
        std::string title, notes, releaseDateString, dueDateString, 
        durationString;
        title = task->getTitle();        
        notes = task->getNotes();        
        boost::posix_time::time_period *interval = task->getInterval();
        releaseDateString = 
        boost::posix_time::to_simple_string(interval->begin());
        dueDateString =
        boost::posix_time::to_simple_string(interval->end());        
        durationString = 
        boost::posix_time::to_simple_string(*task->getDuration());
        boost::property_tree::ptree subtree;
        subtree.put("title", title);
        subtree.put("notes", notes);
        subtree.put("release-date", releaseDateString);
        subtree.put("due-date", dueDateString);
        subtree.put("duration", durationString);
        pt.add_child("tasks.task", subtree);
        delete task;
    }
    
    write_xml(tasksFilename, pt);
    delete workingInterval;
}

boost::posix_time::time_period *Scheduler::getWorkingInterval() {
    return workingInterval;
}

void Scheduler::setWorkingInterval(boost::posix_time::time_period *interval) {
    delete workingInterval; // Delete the current object pointed to by
                            // workingInterval
    workingInterval = interval; // Point it to the new interval
}

void Scheduler::addTask(Task *task) {
    taskList.push_back(task);
}

void Scheduler::deleteTask(int i) {
    if (i - 1 >= taskList.size()) {
        throw std::exception();
    }
    taskList.erase(taskList.begin() + i - 1);
}

Task *Scheduler::getTask(int i) {
    return taskList[i - 1];
}

std::vector<Task *> Scheduler::getTaskList() {
    return taskList;
}
