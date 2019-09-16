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

#ifndef rtcwake_schedule_h
#define rtcwake_schedule_h

#include <algorithm>
#include <cassert>
#include <iterator>

#include <ctime>
#include <iterator>
#include <regex>

#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace rtc
{
using boost::gregorian::days;
using boost::posix_time::hours;
using boost::posix_time::minutes;
using boost::posix_time::seconds;

using boost::gregorian::date;

using time_point_t = boost::posix_time::ptime;
using duration_t = boost::posix_time::time_duration;

inline time_point_t now()
{
	return boost::posix_time::second_clock::local_time();
}

class pipe_handle
{
public:
	explicit pipe_handle(FILE *fp) : m_fp(fp) {}

	operator FILE *() { return m_fp; }

	~pipe_handle()
	{
		if (m_fp)
		{
#ifdef _WIN32
			_pclose(m_fp);
#else
			pclose(m_fp);
#endif
			m_fp = 0;
		}
	}

private:
	FILE *m_fp;
};

struct cmd_t
{
	std::string power_down;
	std::string check_stay_awake;
};

struct action_t
{
	time_point_t on;
	time_point_t off;

	bool operator==(const action_t &rhs) const
	{
		return on == rhs.on && off == rhs.off;
	}
	bool operator!=(const action_t &rhs) const { return !(*this == rhs); }
	bool operator<(const action_t &rhs) const { return on < rhs.on; }
};

// for debugging
std::string to_string(const action_t &action)
{
	std::string s_on = boost::posix_time::to_iso_string(action.on);
	std::string s_off = boost::posix_time::to_iso_string(action.off);
	return s_on + "-" + s_off;
}

time_point_t get_week_start(const time_point_t tp) // aka monday
{
	date day(tp.date());

	date tmp;

	switch (day.day_of_week())
	{
		case boost::gregorian::Monday:
			tmp = day;
			break;
		case boost::gregorian::Tuesday:
			tmp = day - days(1);
			break;
		case boost::gregorian::Wednesday:
			tmp = day - days(2);
			break;
		case boost::gregorian::Thursday:
			tmp = day - days(3);
			break;
		case boost::gregorian::Friday:
			tmp = day - days(4);
			break;
		case boost::gregorian::Saturday:
			tmp = day - days(5);
			break;
		case boost::gregorian::Sunday:
			tmp = day - days(6);
			break;
	}

	return time_point_t(tmp);
}

duration_t to_day_duration(std::string s)
{
	int scale = 0;
	if (s == "Mon")
		scale = 0;
	else if (s == "Tue")
		scale = 1;
	else if (s == "Wed")
		scale = 2;
	else if (s == "Thu")
		scale = 3;
	else if (s == "Fri")
		scale = 4;
	else if (s == "Sat")
		scale = 5;
	else if (s == "Sun")
		scale = 6;
	else
		throw std::runtime_error("to_day_duration(): Unknown day: " + s);

	return hours(scale * 24);
}

duration_t to_hour_duration(std::string s)
{
	std::regex ex("([0-2][0-9]):([0-5][0-9])");
	std::smatch what;
	if (std::regex_match(s, what, ex))
	{
		auto h = std::atol(what[1].str().c_str());
		auto m = std::atol(what[2].str().c_str());
		return hours(h) + minutes(m);
	}
	else
	{
		std::string msg = "to_hour_duration(): Unrecognized string: " + s;
		throw std::runtime_error(msg);
	}
}

template <typename inserter_t>
cmd_t read_schedule(inserter_t inserter, std::istream &is,
					const time_point_t now)
{
	// get the begin of this week
	auto week_start = get_week_start(now);

	// clang-format off
	std::regex ex_action("(Mon|Tue|Wed|Thu|Fri|Sat|Sun):([0-2][0-9]):([0-5][0-9])\\-(Mon|Tue|Wed|Thu|Fri|Sat|Sun):([0-2][0-9]):([0-5][0-9])( |\t|#.*)*");
	// clang-format on
	std::regex ex_comment("^(#.*)|( |\\t)*");
	std::regex ex_stay_awake("CheckStayAwake=(.*)");
	std::regex ex_power_down("PowerDown=(.*)");

	cmd_t cmd;

	std::string line;
	std::smatch what;
	while (std::getline(is, line))
	{
		if (std::regex_match(line, what, ex_action))
		{
			std::string start_day = what[1].str();
			std::string start_time = what[2].str() + ":" + what[3].str();

			std::string end_day = what[4].str();
			std::string end_time = what[5].str() + ":" + what[6].str();

			// convert it to a action
			time_point_t on = week_start;
			time_point_t off = on;

			on += to_hour_duration(start_time);
			on += to_day_duration(start_day);

			off += to_hour_duration(end_time);
			off += to_day_duration(end_day);

			// handle the special case: like "Sun:16:00-Mon:01:00"
			if (off < on)
			{
				// but this also means we need to add on time week_start to off
				inserter = {week_start, off};

				// this is the last entry
				off += hours(7 * 24);
			}

			inserter = {on, off};
		}
		else if (std::regex_match(line, what, ex_comment))
		{
			// skip this comment
		}
		else if (std::regex_match(line, what, ex_power_down))
		{
			// this is the command to power down the PC
			cmd.power_down = what[1].str();
		}
		else if (std::regex_match(line, what, ex_stay_awake))
		{
			// this is the command to power down the PC
			cmd.check_stay_awake = what[1].str();
		}
		else
		{
			std::string msg =
				"read_schedule(): Unrecognized syntax at line: " + line;
			throw std::runtime_error(msg);
		}
	}

	return cmd;
}

template <typename iterator_t>
void check_schedule(iterator_t begin, iterator_t end)
{
	// 1. check on < off and next on > last off
	auto pos1 = std::adjacent_find(
		begin, end, [](const action_t &a1, const action_t &a2) -> bool {
			return a1.off > a2.on;
		});
	if (pos1 != end)
	{
		throw std::runtime_error(
			"check_schedule: next off time < current on time");
	}

	// 2. check that each items on < off
	auto pos2 = std::find_if(begin, end, [](const action_t &a) -> bool {
		bool tmp = a.off > a.on;
		if (!tmp)
			return false;
		else
		{
			// everything ok, if we go to the next week
			assert(a.off > a.on);
			return (a.off - a.on) > hours(24 * 7);
		}
	});
	if (pos2 != end)
	{
		throw std::runtime_error("check_schedule: off time < on time");
	}

	// 3. check for overlaps
	auto pos3 =
		std::find_if(begin, end, [begin, end](const action_t &a) -> bool {
			auto p = std::find_if(begin, end, [a](const action_t &b) -> bool {
				if (a == b) // same action, dont mind
					return false;
				else
				{
					return (a.on >= b.on && a.on <= b.off);
				}
			});

			return p != end;
		});
	if (pos3 != end)
	{
		throw std::runtime_error("check_schedule: overlaping schedules");
	}
}

template <typename iterator_t>
bool get_state(iterator_t begin, iterator_t end, const time_point_t tp)
{
	auto pos_is_on = std::find_if(begin, end, [tp](const action_t &a) -> bool {
		if (a.on < a.off)
		{
			return a.on <= tp && tp < a.off;
		}
		else
		{
			// sunday->monday
			if (a.on <= tp)
				return true;
			else if (tp < a.off)
				return true;
			else
				return false;
		}
	});

	return pos_is_on != end;
}

template <typename iterator_t>
time_point_t get_next_on_time(iterator_t begin, iterator_t end,
							  const time_point_t tp)
{
	// 1. check on < off and next on > last off
	auto pos = std::adjacent_find(
		begin, end, [tp](const action_t &a1, const action_t &a2) -> bool {
			return a1.off <= tp && tp < a2.on;
		});

	if (pos == end)
	{
		// the time is before or after the last entry in the schedule
		auto distance = std::distance(begin, end);
		if (std::distance(begin, end) > 0)
		{
			// is the time before the first entry?
			if (tp < begin->on)
			{
				// ok we need to sleep until the first entry in the schedule
				return begin->on;
			}

			// is the next start in the next week?
			if (tp > (begin + distance)->off)
			{
				// ok: wee need to sleep until the start in the next week
				return begin->on + boost::posix_time::hours(7 * 24);
			}
		}

		// we failed to get the next on time: Print some debug info
		std::string debug;
		debug += "TP: " + boost::posix_time::to_iso_string(tp) + "\n";
		for (iterator_t it = begin; it != end; ++it)
		{
			debug += to_string(*it) + "\n";
		}

		throw std::runtime_error("get_next_on_time: pos == end failed\n" +
								 debug);
	}

	pos++;
	if (pos == end)
	{
		// The last entry reached, go to the forst one
		pos = begin;
	}

	auto a = *pos;
	return a.on;
}

// we return the command. This way we can test the function much easier
template <typename iterator_t>
std::string build_power_off_command(iterator_t begin, iterator_t end,
									cmd_t cmds, const time_point_t now)
{
	// execute the power down command
	auto wake_up_at = get_next_on_time(begin, end, now);
	if (wake_up_at < now)
	{
		std::string msg = "power_off: wake_up_at < now: Software error";
		throw std::runtime_error(msg);
	}

	// build the command
	std::string cmd;
	{
		auto sec_to_sleep = (wake_up_at - now).total_seconds();
		auto s_sec = std::to_string(sec_to_sleep);

		auto pos = cmds.power_down.find("%d");
		if (pos == std::string::npos)
		{
			std::string msg = "power_off: PowerDown needs %d argument: line: " +
							  cmds.power_down;
			throw std::runtime_error(msg);
		}

		std::back_insert_iterator<std::string> cmd_inserter(cmd);
		std::regex ex_replace("%d");
		std::regex_replace(cmd_inserter, cmds.power_down.begin(),
						   cmds.power_down.end(), ex_replace, s_sec);
	}

	return cmd;
}

std::string execute(std::string cmd)
{
#ifdef _WIN32
	pipe_handle stream(_popen(cmd.c_str(), "r"));
#else
	pipe_handle stream(popen(cmd.c_str(), "r"));
#endif

	if (!stream)
	{
		throw std::runtime_error("execute: popen failed");
	}

	// read response until process exits
	std::string response;
	{
		char c;
		while ((c = fgetc(stream)) != EOF)
			response += c;
	}

	return response;
}

bool check_stay_awake(cmd_t cmds, const time_point_t now)
{
	auto response = execute(cmds.check_stay_awake);
	return response != "0\n";
}

} // namespace rtc

#endif // rtcwake_schedule_h
