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

#include <iostream>
#include <sstream>
#include <string>

BOOST_AUTO_TEST_CASE(syntax_error_test)
{
	using rtc::clock_t;
	using namespace rtc;

	const std::string test_schedule =
		"# Schedule xyz\n"
		"\n"
		"Mon:16:00-Tue:01:00\n"
		"Tue:16:00-Wed:01:00\n"
		"Wed:10:35-Thu:01:00\n"
		"Thu:16:00-Fri:01:00\n"
		"Fri:16:00-Sat:01:00\n"
		"Sat:16:xx-Sun:01:00 # Hmmm?\n"
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

	using namespace std::chrono;
	// error reporting is done by exceptions
	std::istringstream iss(test_schedule);
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

BOOST_AUTO_TEST_CASE(main_test)
{
	using rtc::clock_t;
	using namespace rtc;
	
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

	using namespace std::chrono;
	// error reporting is done by exceptions
	std::istringstream iss(test_schedule);
	std::vector<action_t> sched;
	std::back_insert_iterator<decltype(sched)> back_inserter(sched);

	auto cmds = read_schedule(back_inserter, iss);
	BOOST_CHECK(sched.size() == 7);

	BOOST_CHECK(cmds.power_down == "/usr/sbin/rtcwake -m off -s %d");
	BOOST_CHECK(cmds.check_stay_awake == "echo 0");

	std::sort(sched.begin(), sched.end());
	check_schedule(sched.begin(), sched.end());

	// now test for each minute if there is the right thing reported
	auto now = clock_t::now();
	auto test_now = get_week_start(now);
	auto week_start = test_now;
	int cnt = 0;

	// mon: 00:00-00:59
	while (test_now < week_start + hours(1))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// mon 01:00-15:59
	while (test_now < week_start + hours(16))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// mon 16:00- Tue:00:59
	while (test_now < week_start + hours(25))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Tue 01:00- Tue:15:59
	while (test_now < week_start + hours(1 * 24 + 16))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Tue 16:00- wed:00:59
	while (test_now < week_start + hours(2 * 24 + 1))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Wed 01:00- Wed:10:34
	while (test_now < week_start + hours(2 * 24 + 10) + minutes(35))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Wed 10:35- Thu:00:59
	while (test_now < week_start + hours(3 * 24 + 1))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Thu 01:00- Thu:15:59
	while (test_now < week_start + hours(3 * 24 + 16))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Thu 16:00- Fri:00:59
	while (test_now < week_start + hours(4 * 24 + 1))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Fri 01:00- Fri:15:59
	while (test_now < week_start + hours(4 * 24 + 16))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Fri 16:00- Sat:00:59
	while (test_now < week_start + hours(5 * 24 + 1))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Sat 01:00- Sat:15:59
	while (test_now < week_start + hours(5 * 24 + 16))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Sat 16:00- Sun:00:59
	while (test_now < week_start + hours(6 * 24 + 1))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}

	// Sun 01:00- Sun:15:59
	while (test_now < week_start + hours(6 * 24 + 16))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == false);
		test_now += minutes(1);
		++cnt;
	}

	// Sun 16:00- Sun:23:59
	while (test_now < week_start + hours(7 * 24))
	{
		BOOST_CHECK(get_state(sched.begin(), sched.end(), test_now) == true);
		test_now += minutes(1);
		++cnt;
	}
	BOOST_CHECK(cnt == 7 * 24 * 60);
}
