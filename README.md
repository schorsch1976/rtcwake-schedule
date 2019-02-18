# rtcwake-schedule

## Description
rtcwake-schedule is a C++11 scheduling program called by cron. It reads in a schedule and
powers off your NAS or server to preserve energy. If it needs to switch off, it calls
a configurable script to check if it is allowed to power down the PC.

`rtcwake-schedule` is licended under [GPL-3.0](https://www.gnu.org/licenses/gpl-3.0.html)

## Build requirements
- CMake
- C++11 compiler
- [Boost](https://www.boost.org) >=1.62.0
- git
- groff (for the man page)

## Runtime requirements
- [rtcwake](https://linux.die.net/man/8/rtcwake). In [debian](https://www.debian.org) it is in the package util-linux.
- std c++ library

In the config file
~~~~~
# Power down to off state
PowerDown=/usr/sbin/rtcwake -m off -s %d
~~~~~

To shutdown the NAS or Server, rtcwake-schedule executes the configurable
command. `%d` gets replaced by the seconds to stays off.

## Testing requirements
- Boost::unit_test_framework library
- std C++ Library

### Howto run the tests?
In your build directory just type
~~~~~
make tests
~~~~~


## Writing schedules
A schedule is a list of weekday and times.

This is my schedule. The specified times are "on" times.
My NAS is just 68.5h (of 172 h) ~ 40% per week on. This cuts
the electric power need.

You can also wake up your NAS by [WOL](https://en.wikipedia.org/wiki/Wake-on-LAN) and
check in the `CheckStayAwake` script the uptime to prevent the new shutdown.

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
