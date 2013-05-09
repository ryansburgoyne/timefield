/* 
 * timefield-cmd.cpp
 * Ryan Burgoyne
 * 24 Nov 2011
 * TimeField Command Line Interface
 * This file provides the command line interface for using the TimeField 
 * scheduling program.
 */

#include <iostream> // for cin and cout - writing and reading on the command 
                    // line
#include <iomanip> // for padding output with zeros
#include <fstream> // for ifstream - reading the help file
#include <string>
#include <sstream> // for writing working interval to output
#include <map> // for storing the application strings
#include <exception>
#include <vector>

// for loading strings.xml
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include <boost/lexical_cast.hpp> // for converting string to integer
#include <boost/algorithm/string.hpp> // for removing leading whitespace and
                                      // splitting start and end dates

// for date and time operations
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "Scheduler.h" // Manages the tasks
#include "Task.h"

#define TASKS_FILENAME "tasks.xml" // contains the persistent XML task data
#define STRINGS_FILENAME "strings.xml" // contains application strings, 
                                       // separated from the source code 
                                       // to simplify localization
#define HELP_FILENAME "help.txt" // contains help information which can be 
                                 // displayed on the command line

std::string cwd;
std::map<std::string,std::string> strings; // string names are mapped to actual 
                                           // string values

void loadStrings();
void list(Scheduler *scheduler);
boost::posix_time::time_period *
parseInterval(std::string, const boost::posix_time::time_period 
              *currentWorkingInterval);
boost::posix_time::ptime parseDateTime(std::string dateTimeString,
                                       boost::posix_time::time_duration
                                       defaultTime,
                                       int currentYear,
                                       const boost::gregorian::date today);
void newTask(Scheduler *scheduler);
void editTask(Scheduler *scheduler, int id);
void deleteTask(Scheduler *scheduler, int id);
void printTask(Scheduler *scheduler, int id);
void spawnTask(Scheduler *scheduler, int parentId);
void generateSchedule(Scheduler *scheduler);
void showHelp();
std::string prompt(std::string promptText, std::string defaultVal);
std::string buildIntervalString(boost::posix_time::time_period *interval);
std::string getDateTimeString(boost::posix_time::ptime dateTime, 
                              std::string timeString);
std::string getTimeString(boost::posix_time::ptime time);
int getTaskId(std::string input);

class InvalidIntervalException : public std::exception {};

/* Load application strings, then loop through the main menu */
int main(int argc,char *argv[]) {
    std::string pathToExe = std::string(argv[0]);
    int lastSeparator = pathToExe.find_last_of('/');
    cwd = pathToExe.substr(0, lastSeparator + 1);
    // Create the Scheduler object which performs the task management.
    std::stringstream tasksPath;
    tasksPath << cwd << TASKS_FILENAME;
    Scheduler *scheduler = new Scheduler(tasksPath.str());

    // Load user interface strings into a map
    loadStrings();
    std::string input;
    
    std::cout << strings["application-title"] << " " << 
    strings["application-version"] << std::endl;
    // Print the command prompt.
    std::cout << strings["command-prompt"] << std::endl;
    while (true) {
        int id = -1; // ID -1 means no task selected.
        // Print the working interval at the head of each prompt.
        std::cout << buildIntervalString(scheduler->getWorkingInterval()) 
        << " ";
        getline(std::cin, input);
        switch (input[0]) {
            case 'l': // list all tasks
                list(scheduler);
                break;
            case 'c': // change working interval
                      // remove the command character from the string
                try {
                    scheduler->
                    setWorkingInterval(parseInterval(input.substr(1), scheduler->
                                                     getWorkingInterval()));
                }
                catch (std::exception &e) {}
                break;
            case 'n': // new task
                newTask(scheduler);
                break;
            case 'e': // edit task
                id = getTaskId(input);
                if (id != -1) {
                    editTask(scheduler, id);
                }
                break;
            case 'd': // delete task
                id = getTaskId(input);
                if (id != -1) {
                    deleteTask(scheduler, id);
                }
                break;
            case 'p': // print task
                id = getTaskId(input);
                if (id != -1) {
                    printTask(scheduler, id);
                }
                break;
            case 's': // spawn task
                id = getTaskId(input);
                if (id != -1) {
                    spawnTask(scheduler, id);
                }
                break;
            case 'g':
                generateSchedule(scheduler);
                break;
            case 'h': // display help
                showHelp();
                break;
            case 'q': //quit
                delete scheduler;
                return 0;
            default:
                std::cout << strings["invalid-command-error"] << std::endl;
                break;
        }
    }
    delete scheduler;
}

/* List all tasks in the working interval. */
void list(Scheduler *scheduler) {
    std::vector<Task *> list = scheduler->getTaskList();
    for (int i = 0; i < list.size(); i++) {
        if (list[i]->getInterval()->intersects(*scheduler->getWorkingInterval())) 
        {
            // List friendly numbers, starting at 1
            std::cout << i + 1 << "\t" << list[i]->getTitle() << std::endl;
        }
    }
}

/* Prompt for a new working interval and change to it. */
boost::posix_time::time_period *parseInterval(std::string input,
                                              const boost::posix_time::
                                              time_period 
                                              *currentWorkingInterval) {
    // These durations, dates, and periods are used for calculating
    // relative time periods
    const boost::gregorian::date currentBeginDate(currentWorkingInterval->
                                                  begin().date());
    const int dayOfWeek = currentBeginDate.day_of_week();
    const boost::posix_time::time_duration oneDay(boost::posix_time::
                                                   hours(24));    
    const boost::posix_time::time_duration oneWeek(boost::posix_time::hours(24)
                                                   * 7);
    const boost::gregorian::date today(boost::gregorian::day_clock::
                                       local_day());
    
    // These times will be set and passed as the new interval at the end of the
    // function.
    boost::posix_time::ptime begin, end;
    try {
        // Remove extra whitespace from the string
        boost::algorithm::trim(input);
        
        // First, if the input is of the format [[start time] - [end time]], split
        // it into two strings and process them.
        int splitPos = input.find('-');
        if (splitPos != std::string::npos) { // If the string contains a '-'        
            std::string beginString = input.substr(0, splitPos);        
            boost::algorithm::trim(beginString);
            std::string endString = input.substr(splitPos + 1);
            boost::algorithm::trim(endString);
            
            // If no start time is given, the default is midnight (00:00)
            begin = parseDateTime(beginString,
                                  boost::posix_time::hours(0),
                                  (int)currentBeginDate.year(), today);
            // If no end time is given, the default is 23:59
            end = parseDateTime(endString,
                                boost::posix_time::hours(0),
                                (int)currentBeginDate.year(), today);
        }
        else if (input.find('/') != std::string::npos) { 
            // The input is a single date
            int month, day, year;
            int firstSplitPos = input.find('/');
            month = boost::lexical_cast<int>(input.substr(0, firstSplitPos));            
            int secondSplitPos = input.find('/', firstSplitPos + 1);
            if (secondSplitPos != std::string::npos) { // Year provided
                day = boost::lexical_cast<int>(input.
                                               substr(firstSplitPos + 1, 
                                                      secondSplitPos
                                                      - (firstSplitPos + 1)));
                std::string yearString = input.substr(secondSplitPos + 1);
                year = boost::lexical_cast<int>(input.
                                                substr(secondSplitPos + 1));
            }
            else { // No year provided, use the current working year
                day = boost::lexical_cast<int>(input.substr(firstSplitPos
                                                                 + 1));
                year = currentBeginDate.year();
            }
            boost::gregorian::date date(year, month, day);
            begin = boost::posix_time::ptime(date);
            end = begin + oneDay; // Interval ends at 23:59 on the given day.
        }
        else { // The input is a shortcut string
            int splitPos = input.find(' ');
            std::string firstWord = input.substr(0, splitPos);
            std::string secondWord = input.substr(splitPos + 1);
            if (firstWord == strings["today"]) {
                // Set working interval to the current day
                begin = boost::posix_time::ptime(today);
                end = begin + oneDay;
            }
            else if (firstWord == strings["prev"]) {
                if (secondWord == strings["day"]) {
                    begin = boost::posix_time::ptime(currentBeginDate
                                                     - boost::gregorian::days(1));
                    end = begin + oneDay;
                }
                else if (secondWord == strings["week"]) {
                    begin = boost::posix_time::ptime(currentBeginDate - boost::
                                                     gregorian::days(dayOfWeek)
                                                     - boost::gregorian::weeks(1));
                    end = begin + oneWeek;
                }
            }
            else if (firstWord == strings["this"]) {
                if (secondWord == strings["day"]) { // Differs from "today" because
                                                    // it refers to the first day
                                                    // of the current working
                                                    // interval, not the acutal 
                                                    // current date.
                    begin = boost::posix_time::ptime(currentBeginDate);
                    end = begin + oneDay;                
                }
                else if (secondWord == strings["week"]) {
                    begin = boost::posix_time::ptime(currentBeginDate - boost::
                                                     gregorian::days(dayOfWeek));
                    end = begin + oneWeek;
                }
            }
            else if (firstWord == strings["next"]) {
                if (secondWord == strings["day"]) {
                    begin = boost::posix_time::ptime(currentBeginDate
                                                     + boost::gregorian::days(1));
                    end = begin + oneDay;
                }
                else if (secondWord == strings["week"]) {
                    begin = boost::posix_time::ptime(currentBeginDate - boost::
                                                     gregorian::days(dayOfWeek)
                                                     + boost::gregorian::weeks(1));
                    end = begin + oneWeek;
                }
            }
            else {
                throw InvalidIntervalException();
            }

        }
        
        // The duration of the interval must be non-negative, and must be 
        // within the bounds of min_date_time and max_date_time
        if (begin > end || begin < boost::posix_time::min_date_time 
            || end < boost::posix_time::min_date_time
            || begin > boost::posix_time::max_date_time
            || end > boost::posix_time::max_date_time) {
            throw InvalidIntervalException();
        }
        
        return new boost::posix_time::time_period(begin, end);
    }
    catch (std::exception &e) {
        std::cout << strings["invalid-interval-error"] << std::endl;
        throw e;
    }
}

/* 
 * Takes a date-time string, separates the date and time, and parses a ptime
 * object.
 */
boost::posix_time::ptime parseDateTime(std::string dateTimeString,
                                       boost::posix_time::time_duration
                                       defaultTime,
                                       int currentYear,
                                       const boost::gregorian::date today) {
    // Check if the date-time is a special value
    boost::posix_time::ptime dateTime;
    if (dateTimeString == "<") {
        dateTime = boost::posix_time::ptime(boost::posix_time::min_date_time);
    }
    else if (dateTimeString == ">") {
        dateTime = boost::posix_time::ptime(boost::posix_time::max_date_time);
    }
    else if (dateTimeString == "now") {
        dateTime = boost::posix_time::second_clock::local_time();
    }
    else {    
        // Separate the date and time strings
        std::string dateString;
        std::string timeString;
        // If it contains a space, it must have both a date and a time
        int splitPos = dateTimeString.find(' ');
        if (splitPos != std::string::npos) { // If the string contains a ' '
            dateString = dateTimeString.substr(0, splitPos);
            boost::algorithm::trim(dateString);
            timeString = dateTimeString.substr(splitPos + 1);
            boost::algorithm::trim(timeString);            
        }
        else { // Only a date or time is given
               // If it contains a '/', it must be a date
            if (dateTimeString.find('/') != std::string::npos) {
                dateString = dateTimeString;
            }
            // If it contains a ':', it must be a time
            else if (dateTimeString.find(':') != std::string::npos) {
                timeString = dateTimeString;
            }
            else { // If it is neither a date or a time, it is invalid input
                throw InvalidIntervalException();
            }
        }
        
        // Parse the strings into date and time objects
        boost::gregorian::date date;
        if (dateString != "") {
            int month, day, year;
            int firstSplitPos = dateString.find('/');
            month = boost::lexical_cast<int>(dateString.
                                             substr(0, firstSplitPos));            
            int secondSplitPos = dateString.find('/', firstSplitPos + 1);
            if (secondSplitPos != std::string::npos) { // Year provided
                day = boost::
                lexical_cast<int>(dateString.
                                  substr(firstSplitPos + 1, secondSplitPos
                                         - (firstSplitPos + 1)));
                std::string yearString = dateString.substr(secondSplitPos + 1);
                year = boost::lexical_cast<int>(dateString.
                                                substr(secondSplitPos + 1));
            }
            else { // No year provided, use the current working year
                day = boost::lexical_cast<int>(dateString.
                                               substr(firstSplitPos + 1));
                year = currentYear;
            }            
            date = boost::gregorian::date(year, month, day);
        }
        else {
            // If only a time was given, assume the date is today
            date = today;
        }
        
        boost::posix_time::time_duration time;
        if (timeString != "") {
            int firstSplitPos = timeString.find(':');
            int hours = boost::lexical_cast<int>(timeString.
                                                 substr(0, firstSplitPos));
            int minutes = boost::
            lexical_cast<int>(timeString.substr(firstSplitPos + 1));
            time = boost::posix_time::time_duration(hours, minutes, 0);
        }
        else {
            // If only a date was given, assume the default time
            time = defaultTime;
        }
        dateTime = boost::posix_time::ptime(date, time);
    }
        
    return dateTime;
}

/* Prompt for input, then generate a new task. */
void newTask(Scheduler *scheduler) {
    std::string title, notes, intervalString, durationString;
    std::cout << strings["new-task-prompt"] << std::endl;

    try {
        title = prompt(strings["title-prompt"], "");
        notes = prompt(strings["notes-prompt"], "");
        intervalString = prompt(strings["interval-prompt"], "");
        durationString = prompt(strings["duration-prompt"], "");
        
        boost::posix_time::time_period *interval = 
        parseInterval(intervalString, scheduler->getWorkingInterval());
        
        boost::posix_time::time_duration *duration =
        new boost::posix_time::
        time_duration(boost::posix_time::
                      duration_from_string(durationString + ":00"));
        
        scheduler->addTask(new Task(title, notes, interval, duration, NULL));
    }
    catch (...) {
        std::cout << strings["invalid-input-error"] << std::endl;
    }
}

/* Loop through options for working with a selected task. */
void editTask(Scheduler *scheduler, int id) {
    // TODO: Implement this
}

/* Delete a selected task. */
void deleteTask(Scheduler *scheduler, int id) {
    try {
        scheduler->deleteTask(id);
    }
    catch (...) {
        std::cout << strings["invalid-task-error"] << std::endl;
    }
}

/* Print a selected task to the screen */
void printTask(Scheduler *scheduler, int id) {
    try {
        Task *task = scheduler->getTask(id);
        std::cout << task->getTitle() << std::endl
        << task->getNotes() << std::endl
        << buildIntervalString(task->getInterval()) << std::endl
        << *task->getDuration() << std::endl;
    }
    catch (...) {
        std::cout << strings["invalid-task-error"] << std::endl;
    }
}

/* Spawn a new task as a child of a selected task. */
void spawnTask(Scheduler *scheduler, int parentId) {
}

/* Generate and display a schedule for the working interval. */
void generateSchedule(Scheduler *scheduler) {
}

/* Show a help file. */
void showHelp() {
    std::string line;
    std::stringstream helpPath;
    helpPath << cwd << HELP_FILENAME;
    std::ifstream helpFile(helpPath.str().c_str());
    if (helpFile.is_open()) {
        while ( helpFile.good() )
        {
            getline (helpFile,line);
            std::cout << line << std::endl;
        }
        helpFile.close();
    }
}

/* Load strings from an XML file into a map. */
void loadStrings() {
    boost::property_tree::ptree pt;
    std::stringstream stringsPath;
    stringsPath << cwd << STRINGS_FILENAME;
    read_xml(stringsPath.str(), pt);
    BOOST_FOREACH(boost::property_tree::ptree::value_type &v, 
                  pt.get_child("strings"))
    if (v.first == "string") {        
        std::string key = v.second.get<std::string>("<xmlattr>.name");
        std::string value = v.second.data();
        strings[key] = value;
    }
}

// Builds a string to output on the command prompt
std::string buildIntervalString(boost::posix_time::time_period *interval) {
    boost::posix_time::ptime begin, end;
    begin = interval->begin();
    end = interval->end();
    //end -= boost::posix_time::minutes(1); // The interval ends one minute
                                          // before the end time - the end time
                                          // is not included in the interval.
    std::stringstream intervalStringStream;

    std::string beginTimeString = getTimeString(begin);
    std::string endTimeString = getTimeString(end);
    
    bool fullDay = false;
    // Don't show the times if the interval only contains full days.
    // max_date_time ends at 23:59 so it needs a special case.
    if (beginTimeString.find("00:00") != std::string::npos
        && (endTimeString.find("00:00") != std::string::npos
            || end == boost::posix_time::max_date_time)) {
        beginTimeString = "";
        endTimeString = "";
        fullDay = true;
    }

    std::string beginDateTimeString = getDateTimeString(begin, beginTimeString);
    std::string endDateTimeString = getDateTimeString(end, endTimeString);
    
    intervalStringStream << "[" << beginDateTimeString;
    
    // Don't show the end date if it is a single full day.
    if (!(begin.date() + boost::gregorian::days(1) == end.date() && fullDay)) {
        intervalStringStream << " - " << endDateTimeString;
    } 
    intervalStringStream << "]";
    
    return intervalStringStream.str();
}

// Outputs properly formatted date string
std::string getDateTimeString(boost::posix_time::ptime dateTime, 
                              std::string timeString) {
    std::string dateTimeString;
    if (dateTime == boost::posix_time::min_date_time) {
        dateTimeString = "<";
    }
    else if (dateTime == boost::posix_time::max_date_time) {
        dateTimeString = ">";
    }
    else {
        std::stringstream dateTimeStringStream;
        dateTimeStringStream << dateTime.date().day() << " " 
        << dateTime.date().month() << " " << dateTime.date().year() 
        << timeString;
        dateTimeString = dateTimeStringStream.str();
    }
    return dateTimeString;
}
    
// Outputs properly formatted time
std::string getTimeString(boost::posix_time::ptime time) {
    std::stringstream timeStringStream;
    timeStringStream << std::setfill('0');
    timeStringStream << " " << std::setw(2) 
    << time.time_of_day().hours() << ":" << std::setw(2)
    << time.time_of_day().minutes();
    return timeStringStream.str();
}

/* Write the prompt to the screen with a default value, return the response. */
std::string prompt(std::string promptText, std::string defaultVal) {
    std::string response;
    std::cout << promptText << "[" << defaultVal << "]: ";
    getline(std::cin, response);
    
    // Return the default if no response is given.
    if (response == "") {
        response = defaultVal;
    }
    return response;
}

/* 
 * Get a task ID either from the initial command prompt or by prompting for it
 * specifically.
 */
int getTaskId (std::string input) {
    int id = -1;
    try {
        // Try to read the id from the inital command.
        // Remove any leading white space.
        std::string idString = boost::algorithm::trim_left_copy(input.
                                                                  substr(1));
        id = boost::lexical_cast<int>(idString);
    }
    catch (...) {
        std::cout << strings["invalid-task-error"] << std::endl;
    }
    return id;
}
    

    