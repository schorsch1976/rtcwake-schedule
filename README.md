# rtcwake-schedule

rtcwake-schedule is a C++ scheduling program called by cron. It reads in a schedule and
powers off your NAS or server to preserve energy.

How to write the schedule should be obious. See the example folder. 

If the schedule is to power off but there are still network connections open add

~~~~~
# Check Stay awake
# An output of <> "0", not ret_val, output on stdout, let it stay awake
CheckStayAwake=netstat -n | grep tcp | wc -l

# Power down to off state
PowerDown=/usr/sbin/rtcwake -m off -s %d
~~~~~

to the schedule
