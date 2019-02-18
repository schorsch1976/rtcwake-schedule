# rtcwake-schedule

## Description
rtcwake-schedule is a C++ scheduling program called by cron. It reads in a schedule and
powers off your NAS or server to preserve energy.

## Build requirements
- CMake
- C++11 compiler
- Boost 1.62.0

## Runtime requirements
- rtcwake
- boost::date_time and boost::system

To shutdown the NAS or Server, rtcwake-schedule executes the configurable
command. `%d` gets replaced by the seconds to stays off.

~~~~~
# Power down to off state
PowerDown=/usr/sbin/rtcwake -m off -s %d
~~~~~

## Writing schedules
A Schedule is a list of weekday and times.

This is my schedule. The specified times are "on" times.
~~~~~
Mon:16:00-Mon:23:00
Tue:16:00-Wed:01:00
Wed:10:35-Thu:01:00
Thu:16:00-Fri:01:00
Fri:16:00-Sat:03:00 # foldersync time
Sat:16:00-Sun:01:00
Sun:16:00-Mon:01:00
~~~~~

## Check Stay Awake?
If the schedule is to power off but there are still network connections open add

~~~~~
# Check Stay awake
# An output of <> "0", not ret_val, output on stdout, let it stay awake
CheckStayAwake=netstat -n | grep tcp | wc -l
~~~~~

to the schedule.
