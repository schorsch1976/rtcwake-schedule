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

// helper function to debug the schedule
template<typename interator>
std::vector<std::pair<std::string, std::string>> to_string(interator start, interator end)
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
		auto cmds = read_schedule(back_inserter, iss);
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

	read_schedule(back_inserter, iss);

	for (auto& c : sched)
	{
		BOOST_CHECK(c.on < c.off);
	}
}
BOOST_AUTO_TEST_CASE(get_week_start_test)
{
	// simple
	time_point_t now = boost::posix_time::time_from_string("2019-02-24 12:43:12");
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
		time_point_t next_week_now = week_start + boost::posix_time::hours(7 * 24);
		time_point_t next_week_start = get_week_start(next_week_now);
		std::string ws = boost::posix_time::to_iso_string(week_start + hours(7 * 24));
		BOOST_CHECK(ws == "20190225T000000");
	}
}

BOOST_AUTO_TEST_CASE(get_next_on_time_test_1)
{
	// error reporting is done by exceptions
	std::istringstream iss(test_schedule);
	std::vector<action_t> sched;
	std::back_insert_iterator<decltype(sched)> back_inserter(sched);

	auto cmds = read_schedule(back_inserter, iss);
	BOOST_CHECK(sched.size() > 0);

	auto debug_schedule = to_string(sched.begin(), sched.end());

	BOOST_CHECK(cmds.power_down == "/usr/sbin/rtcwake -m off -s %d");
	BOOST_CHECK(cmds.check_stay_awake == "echo 0");

	std::sort(sched.begin(), sched.end());
	check_schedule(sched.begin(), sched.end());

	// Tue: we must be off
	time_point_t now = boost::posix_time::time_from_string("2019-02-19 12:43:12");
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

	auto cmds = read_schedule(back_inserter, iss);
	BOOST_CHECK(sched.size() > 0);

	auto debug_schedule = to_string(sched.begin(), sched.end());

	BOOST_CHECK(cmds.power_down == "/usr/sbin/rtcwake -m off -s %d");
	BOOST_CHECK(cmds.check_stay_awake == "echo 0");

	std::sort(sched.begin(), sched.end());
	check_schedule(sched.begin(), sched.end());

	// Tue: we must be off
	time_point_t now = boost::posix_time::time_from_string("2019-02-18 10:57:00");
	BOOST_CHECK(get_state(sched.begin(), sched.end(), now) == false);

	time_point_t next_on = get_next_on_time(sched.begin(), sched.end(), now);
	std::string s_next_on = to_iso_string(next_on);
	BOOST_CHECK(s_next_on == "20190218T160000");
}

BOOST_AUTO_TEST_CASE(main_test)
{
	// error reporting is done by exceptions
	std::istringstream iss(test_schedule);
	std::vector<action_t> sched;
	std::back_insert_iterator<decltype(sched)> back_inserter(sched);

	auto cmds = read_schedule(back_inserter, iss);
	BOOST_CHECK(sched.size() > 0);

	auto debug_schedule = to_string(sched.begin(), sched.end());

	BOOST_CHECK(cmds.power_down == "/usr/sbin/rtcwake -m off -s %d");
	BOOST_CHECK(cmds.check_stay_awake == "echo 0");

	std::sort(sched.begin(), sched.end());
	check_schedule(sched.begin(), sched.end());

	// now test for each minute if there is the right thing reported
	auto test_now = get_week_start(rtc::now());
	auto week_start = test_now;


	std::string ns = boost::posix_time::to_simple_string(rtc::now());
	std::string tns = boost::posix_time::to_simple_string(test_now);
	std::string ws = boost::posix_time::to_simple_string(week_start);
	int cnt = 0;

	// mon: 00:00-00:59
	while (test_now < week_start + hours(1))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// mon 01:00-15:59
	while (test_now < week_start + hours(16))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// mon 16:00- Tue:00:59
	while (test_now < week_start + hours(25))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Tue 01:00- Tue:15:59
	while (test_now < week_start + hours(1 * 24 + 16))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Tue 16:00- wed:00:59
	while (test_now < week_start + hours(2 * 24 + 1))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Wed 01:00- Wed:10:34
	while (test_now < week_start + hours(2 * 24 + 10) + minutes(35))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Wed 10:35- Thu:00:59
	while (test_now < week_start + hours(3 * 24 + 1))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Thu 01:00- Thu:15:59
	while (test_now < week_start + hours(3 * 24 + 16))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Thu 16:00- Fri:00:59
	while (test_now < week_start + hours(4 * 24 + 1))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Fri 01:00- Fri:15:59
	while (test_now < week_start + hours(4 * 24 + 16))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Fri 16:00- Sat:00:59
	while (test_now < week_start + hours(5 * 24 + 1))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Sat 01:00- Sat:15:59
	while (test_now < week_start + hours(5 * 24 + 16))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Sat 16:00- Sun:00:59
	while (test_now < week_start + hours(6 * 24 + 1))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Sun 01:00- Sun:15:59
	while (test_now < week_start + hours(6 * 24 + 16))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Sun 16:00- Sun:24:00
	while (test_now <= week_start + hours(7 * 24))
	{
		std::string s_now = boost::posix_time::to_iso_string(test_now);
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}
	BOOST_CHECK(cnt == 7 * 24 * 60 );
}
