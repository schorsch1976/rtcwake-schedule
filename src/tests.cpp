/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define BOOST_TEST_MODULE "rtcwake-schedule-test"
#include <boost/test/unit_test.hpp>
namespace utf = boost::unit_test;

#include "rtcwake-schedule.h"
using namespace rtc;

#include <iostream>
#include <sstream>
#include <string>

// test schedule
const std::string test_schedule =
	"# Schedule xyz\n"
	"\n"
	"Mon:16:00-Tue:01:00\n"
	"Tue:16:00-Wed:01:00\n"
	"Wed:10:35-Thu:01:00\n"
	"Thu:16:00-Fri:01:00\n"
	"Fri:16:00-Sat:01:00\n"
	"Sat:16:00-Sun:01:00 # Hmmm?\n"
	"\n"
	"# special case\n"
	"Sun:16:00-Mon:01:00\n"
	"\n"
	"# Check Stay awake\n"
	"# An output of <> '0', not ret_val, output on\n"
	"# stdout, let it stay awake\n"
	"CheckStayAwake=echo 0\n"
	"# Power down to off state\n"
	"PowerDown=/usr/sbin/rtcwake -m off -s %d\n";

const std::string test_schedule2 =
	"# Schedule xyz\n"
	"\n"
	"Mon:09:00-Mon:23:00\n"
	"Tue:09:00-Tue:23:00\n"
	"Wed:09:00-Wed:23:00\n"
	"Thu:09:00-Thu:23:00\n"
	"Fri:09:00-Sat:03:00\n"
	"Sat:09:00-Sat:23:00\n"
	"Sun:09:00-Sun:23:00\n"
	"\n"
	"# Check Stay awake\n"
	"# An output of <> 0, not ret_val, output on stdout, let it stay awake\n"
	"CheckStayAwake=netstat -n | grep tcp | grep -v TIME_WAIT | wc -l\n"
	"#CheckStayAwake=echo 1\n"
	"\n"
	"# Power down to off state  \n"
	"PowerDown=/usr/sbin/rtcwake -m off -s %d\n";

// helper function
std::string expected_shutdown_cmd(const time_point_t now,
								  const time_point_t end,
								  std::string power_down_cmd,
								  rtc::action_t first_action,
								  rtc::action_t last_action)
{
	auto sec_to_sleep = (end - now).total_seconds();
	auto s_sec = std::to_string(sec_to_sleep);

	if (now >= last_action.off)
	{
		const time_point_t wake_up_at =
			first_action.on + boost::posix_time::hours(7 * 24);
		sec_to_sleep = (wake_up_at - now).total_seconds();
		s_sec = std::to_string(sec_to_sleep);
	}

	auto pos = power_down_cmd.find("%d");
	if (pos == std::string::npos)
	{
		std::string msg =
			"expected_shutdown_cmd: PowerDown needs %d argument: line: " +
			power_down_cmd;
		throw std::runtime_error(msg);
	}

	std::string ret;
	std::back_insert_iterator<std::string> cmd_inserter(ret);
	std::regex ex_replace("%d");
	std::regex_replace(cmd_inserter, power_down_cmd.begin(),
					   power_down_cmd.end(), ex_replace, s_sec);

	return ret;
}
// helper function to debug the schedule
template <typename interator>
std::vector<std::pair<std::string, std::string>> to_string(interator start,
														   interator end)
{
	// debug the schedule
	std::vector<std::pair<std::string, std::string>> test;
	while (start < end)
	{
		std::string s1, s2;
		s1 = boost::posix_time::to_iso_string(start->on);
		s2 = boost::posix_time::to_iso_string(start->off);
		test.push_back(std::make_pair(s1, s2));

		++start;
	}

	return test;
}

BOOST_AUTO_TEST_CASE(syntax_error_test)
{
	// error reporting is done by exceptions
	std::string syntax_error_schedule = test_schedule;
	syntax_error_schedule += "\n";
	syntax_error_schedule += "Sat:16:xx-Sun:01:00 # Hmmm?";

	std::istringstream iss(syntax_error_schedule);
	std::vector<action_t> sched;
	std::back_insert_iterator<decltype(sched)> back_inserter(sched);

	try
	{
		auto cmds = read_schedule(back_inserter, iss, rtc::now());
		BOOST_REQUIRE(false);
	}
	catch (const std::exception &ex)
	{
		std::string msg = ex.what();
		BOOST_CHECK(msg == "read_schedule(): Unrecognized syntax at line: "
						   "Sat:16:xx-Sun:01:00 # Hmmm?");
	}
}

BOOST_AUTO_TEST_CASE(on_smaller_off)
{
	std::istringstream iss(test_schedule);
	std::vector<action_t> sched;
	std::back_insert_iterator<decltype(sched)> back_inserter(sched);

	read_schedule(back_inserter, iss, rtc::now());

	for (auto &c : sched)
	{
		BOOST_CHECK(c.on < c.off);
	}
}
BOOST_AUTO_TEST_CASE(get_week_start_test)
{
	// simple
	time_point_t now =
		boost::posix_time::time_from_string("2019-02-24 12:43:12");
	time_point_t week_start = get_week_start(now);

	std::string ws = boost::posix_time::to_iso_string(week_start);
	BOOST_CHECK(ws == "20190218T000000");

	// check the whole week
	{
		for (int i = 0; i < 7 * 24; ++i)
		{
			time_point_t ws_now = get_week_start(week_start + hours(i));
			std::string ws = boost::posix_time::to_iso_string(ws_now);
			BOOST_CHECK(ws == "20190218T000000");
		}
	}

	// check next week
	{
		time_point_t next_week_now =
			week_start + boost::posix_time::hours(7 * 24);
		time_point_t next_week_start = get_week_start(next_week_now);
		std::string ws =
			boost::posix_time::to_iso_string(week_start + hours(7 * 24));
		BOOST_CHECK(ws == "20190225T000000");
	}
}

BOOST_AUTO_TEST_CASE(get_next_on_time_test_1)
{
	// error reporting is done by exceptions
	std::istringstream iss(test_schedule);
	std::vector<action_t> sched;
	std::back_insert_iterator<decltype(sched)> back_inserter(sched);

	time_point_t now =
		boost::posix_time::time_from_string("2019-02-19 12:43:12");

	auto cmds = read_schedule(back_inserter, iss, now);
	BOOST_CHECK(sched.size() > 0);

	auto debug_schedule = to_string(sched.begin(), sched.end());

	BOOST_CHECK(cmds.power_down == "/usr/sbin/rtcwake -m off -s %d");
	BOOST_CHECK(cmds.check_stay_awake == "echo 0");

	std::sort(sched.begin(), sched.end());
	check_schedule(sched.begin(), sched.end());

	// Tue: we must be off
	BOOST_CHECK(get_state(sched.begin(), sched.end(), now) == false);

	time_point_t next_on = get_next_on_time(sched.begin(), sched.end(), now);
	std::string s_next_on = to_iso_string(next_on);
	BOOST_CHECK(s_next_on == "20190219T160000");
}

BOOST_AUTO_TEST_CASE(get_next_on_time_test_2)
{
	// error reporting is done by exceptions
	std::istringstream iss(test_schedule);
	std::vector<action_t> sched;
	std::back_insert_iterator<decltype(sched)> back_inserter(sched);

	time_point_t now =
		boost::posix_time::time_from_string("2019-02-18 10:57:00");

	auto cmds = read_schedule(back_inserter, iss, now);
	BOOST_CHECK(sched.size() > 0);

	auto debug_schedule = to_string(sched.begin(), sched.end());

	BOOST_CHECK(cmds.power_down == "/usr/sbin/rtcwake -m off -s %d");
	BOOST_CHECK(cmds.check_stay_awake == "echo 0");

	std::sort(sched.begin(), sched.end());
	check_schedule(sched.begin(), sched.end());

	// Tue: we must be off
	BOOST_CHECK(get_state(sched.begin(), sched.end(), now) == false);

	time_point_t next_on = get_next_on_time(sched.begin(), sched.end(), now);
	std::string s_next_on = to_iso_string(next_on);
	BOOST_CHECK(s_next_on == "20190218T160000");
}

BOOST_AUTO_TEST_CASE(get_next_on_time_test_3)
{
	// error reporting is done by exceptions
	std::istringstream iss(test_schedule);
	std::vector<action_t> sched;
	std::back_insert_iterator<decltype(sched)> back_inserter(sched);

	time_point_t now =
		boost::posix_time::time_from_string("2019-02-28 10:57:00");

	auto cmds = read_schedule(back_inserter, iss, now);
	BOOST_CHECK(sched.size() > 0);

	auto debug_schedule = to_string(sched.begin(), sched.end());

	std::sort(sched.begin(), sched.end());
	check_schedule(sched.begin(), sched.end());

	// Tue: we must be off
	BOOST_CHECK(get_state(sched.begin(), sched.end(), now) == false);

	time_point_t next_on = get_next_on_time(sched.begin(), sched.end(), now);
	std::string s_next_on = to_iso_string(next_on);
	BOOST_CHECK(s_next_on == "20190228T160000");
}

time_point_t RunTest(const rtc::cmd_t cmds, const std::vector<action_t> sched,
					 const time_point_t start, const duration_t duration,
					 const bool expected_state, int &cnt)
{
	auto test_now = start;
	auto test_end = start + duration;
	std::string s_end = boost::posix_time::to_iso_string(test_end);
	while (test_now < test_end)
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);

		bool state_from_schedule =
			get_state(sched.begin(), sched.end(), test_now);
		BOOST_CHECK(state_from_schedule == expected_state);

		if (!expected_state)
		{
			auto power_off_cmd = build_power_off_command(
				sched.begin(), sched.end(), cmds, test_now);

			auto expected =
				expected_shutdown_cmd(test_now, test_end, cmds.power_down,
									  *sched.begin(), *sched.rbegin());
			if (power_off_cmd != expected)
			{
				auto debug = to_string(std::begin(sched), std::end(sched));
				int t = 1;
				expected =
					expected_shutdown_cmd(test_now, test_end, cmds.power_down,
										  *sched.begin(), *sched.rbegin());
			}
			BOOST_CHECK(power_off_cmd == expected);
		}

		test_now += minutes(1);
		++cnt;
	}

	return test_end;
}

BOOST_AUTO_TEST_CASE(main_test)
{
	// error reporting is done by exceptions
	std::istringstream iss(test_schedule);
	std::vector<action_t> sched;
	std::back_insert_iterator<decltype(sched)> back_inserter(sched);

	auto now = rtc::now();
	auto cmds = read_schedule(back_inserter, iss, now);
	BOOST_CHECK(sched.size() > 0);

	auto debug_schedule = to_string(sched.begin(), sched.end());

	BOOST_CHECK(cmds.power_down == "/usr/sbin/rtcwake -m off -s %d");
	BOOST_CHECK(cmds.check_stay_awake == "echo 0");

	std::sort(sched.begin(), sched.end());
	check_schedule(sched.begin(), sched.end());

	// now test for each minute if there is the right thing reported
	auto test_now = get_week_start(now);
	auto week_start = test_now;

	std::string ns = boost::posix_time::to_simple_string(rtc::now());
	std::string tns = boost::posix_time::to_simple_string(test_now);
	std::string ws = boost::posix_time::to_simple_string(week_start);
	int cnt = 0;

	// mon: 00:00-00:59
	test_now = RunTest(cmds, sched, test_now, hours(1), true, cnt);

	// mon: 01:00-15:59
	test_now = RunTest(cmds, sched, test_now, hours(15), false, cnt);

	// mon 16:00-Tue:00:59
	test_now = RunTest(cmds, sched, test_now, hours(9), true, cnt);

	// tue 01:00- Tue:15:59
	test_now = RunTest(cmds, sched, test_now, hours(15), false, cnt);

	// tue 16:00- Wed:00:59
	test_now = RunTest(cmds, sched, test_now, hours(9), true, cnt);

	// Wed:01:00 - Wed:10:34
	test_now =
		RunTest(cmds, sched, test_now, hours(9) + minutes(35), false, cnt);

	// Wed:10:35 - Thu:00:59
	test_now =
		RunTest(cmds, sched, test_now, hours(14) + minutes(25), true, cnt);

	// Thu 01:00- Thu:15:59
	test_now = RunTest(cmds, sched, test_now, hours(15), false, cnt);

	// Thu 16:00- Fri:00:59
	test_now = RunTest(cmds, sched, test_now, hours(9), true, cnt);

	// Fri 01:00- Fri:15:59
	test_now = RunTest(cmds, sched, test_now, hours(15), false, cnt);

	// Fri 16:00- Sat:00:59
	test_now = RunTest(cmds, sched, test_now, hours(9), true, cnt);

	// Sat 01:00- Sat:15:59
	test_now = RunTest(cmds, sched, test_now, hours(15), false, cnt);

	// Sat:16:00-Sun:00:59
	test_now = RunTest(cmds, sched, test_now, hours(9), true, cnt);

	// Sun:01:00-Sun:15:59
	test_now = RunTest(cmds, sched, test_now, hours(15), false, cnt);

	// Sun:16:00-Mon:00:59
	test_now = RunTest(cmds, sched, test_now, hours(9), true, cnt);

	BOOST_CHECK(cnt == 7 * 24 * 60 + 60);
}

BOOST_AUTO_TEST_CASE(main_test2)
{
	// error reporting is done by exceptions
	std::istringstream iss(test_schedule2);
	std::vector<action_t> sched;
	std::back_insert_iterator<decltype(sched)> back_inserter(sched);

	auto now = rtc::now();
	auto cmds = read_schedule(back_inserter, iss, now);
	BOOST_CHECK(sched.size() > 0);

	auto debug_schedule = to_string(sched.begin(), sched.end());

	BOOST_CHECK(cmds.power_down == "/usr/sbin/rtcwake -m off -s %d");
	BOOST_CHECK(cmds.check_stay_awake ==
				"netstat -n | grep tcp | grep -v TIME_WAIT | wc -l");

	std::sort(sched.begin(), sched.end());
	check_schedule(sched.begin(), sched.end());

	// now test for each minute if there is the right thing reported
	auto test_now = get_week_start(now);
	auto week_start = test_now;

	std::string ns = boost::posix_time::to_simple_string(rtc::now());
	std::string tns = boost::posix_time::to_simple_string(test_now);
	std::string ws = boost::posix_time::to_simple_string(week_start);
	int cnt = 0;

	// mon: 00:00-08:59
	test_now = RunTest(cmds, sched, test_now, hours(9), false, cnt);

	// mon 09:00-22:59
	test_now = RunTest(cmds, sched, test_now, hours(14), true, cnt);

	// mon 23:00- Tue:08:59
	test_now = RunTest(cmds, sched, test_now, hours(10), false, cnt);

	// Tue 09:00- Tue:22:59
	test_now = RunTest(cmds, sched, test_now, hours(14), true, cnt);

	// Tue 23:00- wed:08:59
	test_now = RunTest(cmds, sched, test_now, hours(10), false, cnt);

	// Wed 09:00- Wed:22:59
	test_now = RunTest(cmds, sched, test_now, hours(14), true, cnt);

	// Wed 23:00- Thu:08:59
	test_now = RunTest(cmds, sched, test_now, hours(10), false, cnt);

	// Thu 09:00- Thu:22:59
	test_now = RunTest(cmds, sched, test_now, hours(14), true, cnt);

	// Thu 23:00- Fri:08:59
	test_now = RunTest(cmds, sched, test_now, hours(10), false, cnt);

	// Fri 09:00- Sat:02:59
	test_now = RunTest(cmds, sched, test_now, hours(18), true, cnt);

	// Sat 03:00- Sat:08:59
	test_now = RunTest(cmds, sched, test_now, hours(6), false, cnt);

	// Sat 09:00- Sat:22:59
	test_now = RunTest(cmds, sched, test_now, hours(14), true, cnt);

	// Sat 23:00- Sun:08:59
	test_now = RunTest(cmds, sched, test_now, hours(10), false, cnt);

	// Sun 09:00- Sun:22:59
	test_now = RunTest(cmds, sched, test_now, hours(14), true, cnt);

	// Sun 23:00- Sun:24:00
	test_now = RunTest(cmds, sched, test_now, hours(1), false, cnt);

	// Sun 24:00 - Mon:01:00
	test_now = RunTest(cmds, sched, test_now, hours(1), false, cnt);

	BOOST_CHECK(cnt == 7 * 24 * 60 + 60);
}
