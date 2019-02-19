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

#include "rtcwake-schedule.h"

#include <vector>

#include <fstream>
#include <iomanip>
#include <iostream>

namespace ours
{
void usage()
{
	// clang-format off
	std::cout
		<< "\nrtcwake-schedule " << GIT_VERSION << ": (c) Georg Gast <georg@schorsch-tech.de>\n\n"
		<< "Licence: GPL-3.0\n"
		<< "\tThis program is distributed in the hope that it will be useful,\n"
		<< "\tbut WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		<< "\tMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the\n"
		<< "\tGNU General Public License for more details.\n\n"
		<< "Usage:\n\tRun it from cron to read the schedule '" << RC_FILE_PATH << "' and execute the\n"
		<< "\tcommand given at CheckStayAwake=... and PowerDown=...\n"
		<< "\tIf CheckStayAwake prints on it's stdout something different than \"0\" it stays\n"
		<< "\tawake even when its in the off time of the schedule. You can use that, to check for\n"
		<< "\topen connections on your NAS or server.\n\n"

		<< "Options:\n"
		<< "\t-h or --help\tPrint the usage information\n"
		<< "\t-f or --force\tforce the shutdown even the CheckStayAwake reports !=0\n"
		<< "\t-t or --test\ttest the configuration. Print the actions and states\n\n"
		<< std::endl;

	// clang-format on
}

// we dont add, for this minimal amount of options,
// boost::program_options...
enum class mode_t
{
	op = 0,
	forced,
	test,
	usage
};

struct options
{
	mode_t mode = mode_t::op;
	bool forced = false;
};

options parse_options(int argc, char *argv[])
{
	options opts;
	if (argc == 1) // no arguments
	{
		return opts;
	}

	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "-t" || arg == "--test")
		{
			opts.mode = mode_t::test;
		}
		else if (arg == "-f" || arg == "--force")
		{
			opts.forced = true;
		}
		else if (arg == "-h" || arg == "--help")
		{
			opts.mode = mode_t::usage;
			opts.forced = false;
			return opts;
		}
		else
		{
			// any unhandled case, usage
			opts.mode = mode_t::usage;
			opts.forced = false;
			return opts;
		}
	}

	return opts;
}

} // ns ours

int main(int argc, char *argv[])
{
	using namespace rtc;
	using namespace ours;
	using ours::mode_t;
	try
	{
		auto opts = parse_options(argc, argv);
		switch (opts.mode)
		{
			case mode_t::usage:
				usage();
				return EXIT_FAILURE;

			case mode_t::test:
			default:
				// fall through
				break;
		}

		// real work
		std::ifstream ifs(RC_FILE_PATH);

		std::vector<action_t> sched;
		std::back_insert_iterator<decltype(sched)> back_inserter(sched);

		if (opts.mode == mode_t::test)
		{
			std::clog << "Read schedule ..." << std::endl;
		}
		auto cmds = read_schedule(back_inserter, ifs);
		std::sort(sched.begin(), sched.end());

		if (opts.mode == mode_t::test)
		{
			std::clog << "Check schedule ..." << std::endl;
		}
		check_schedule(sched.begin(), sched.end());

		if (opts.mode == mode_t::test)
		{
			std::clog << "Schedule has " << sched.size() << " entries"
					  << std::endl;
		}

		// only execute when we got a schedule. Report error when there is
		// no entry in the schedule
		if (sched.empty())
		{
			throw std::runtime_error("Empty schedule");
		}

		auto now = rtc::now();
		auto state = get_state(sched.begin(), sched.end(), now);

		if (opts.mode == mode_t::test)
		{
			std::clog << "Current state after time: " << std::boolalpha << state
					  << std::endl;
		}

		// we could enlength the time to stay awake when this command
		// returns != "0"
		if (!state)
		{
			state = check_stay_awake(cmds, now);

			// force the shutdown?
			if (opts.forced)
			{
				state = false;
			}
		}
		if (opts.mode == mode_t::test)
		{
			std::clog << "Current state after CheckStayAwake: "
					  << std::boolalpha << state << std::endl;
		}

		if (!state)
		{
			// we need to shut down
			auto power_off_cmd =
				build_power_off_command(sched.begin(), sched.end(), cmds, now);

			switch (opts.mode)
			{
				case mode_t::op:
					execute(power_off_cmd.c_str());
					break;

				case mode_t::test:
				default:
					std::clog << "Would now execute PowerDown script: "
							  << power_off_cmd << std::endl;
					break;
			}
		}

		return EXIT_SUCCESS;
	}
	catch (const std::exception &ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Unknown exception" << std::endl;
	}

	return EXIT_FAILURE;
}
