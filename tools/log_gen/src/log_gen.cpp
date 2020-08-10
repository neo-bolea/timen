#define _CRT_NO_TIME_T
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <assert.h>
#include <direct.h>
#include <fstream>
#include <random>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef unsigned int uint;
typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

typedef float f32;
typedef double f64;

typedef ui64 time_t;
typedef i64 stime_t;

auto log_sep = ',';

void main()
{
	assert(_chdir("..\\data") == 0);

	std::ifstream sym_file("Log.sym");
	std::string cur_proc_str;
	std::vector<std::string> proc_names;
	proc_names.push_back("Idle");
	while (sym_file >> cur_proc_str)
	{
		proc_names.push_back(cur_proc_str);
	}

	ULARGE_INTEGER year_begin_2020;
	{
		FILETIME FileTime2020;
		SYSTEMTIME SysTime2020 = {};
		SysTime2020.wYear = 2020;
		SysTime2020.wMonth = 1;
		SysTime2020.wDay = 1;
		SystemTimeToFileTime(&SysTime2020, &FileTime2020);
		year_begin_2020.LowPart = FileTime2020.dwLowDateTime;
		year_begin_2020.HighPart = FileTime2020.dwHighDateTime;
	}
	// Converts 1s to 100ns (Windows file time unit).
	time_t s_to_file_time = 1000LL * 1000LL * 10LL;
	const time_t time_div_s = 3600 * s_to_file_time;
	time_t time_now = year_begin_2020.QuadPart;

	std::mt19937 mt_rand;
	std::uniform_int_distribution<> index_dist(0, static_cast<int>(proc_names.size() - 1));
	std::uniform_int_distribution<time_t> time_dist((time_t)(0.1 * s_to_file_time), (time_t)(100.0 * s_to_file_time));

	std::bernoulli_distribution interrupt_p(0.01);
	std::uniform_int_distribution<time_t> interrupt_time((time_t)(10 * s_to_file_time), (time_t)(24 * 3600 * s_to_file_time));

	std::uniform_real_distribution<f32> time_mult_dist(1.0f / 1.1f, 1.1f);
	f32 max_time_mult = 10.0f;
	std::vector<f32> time_proc_mults(proc_names.size());
	/*
		If we just assign a random time to each activity, we would get very random patterns.
		If we can make the patterns more predictable it will be easier to see bugs.
		Thus we add an additional time multiplier on a per-program basis.
		That way some programs always tend to take up larger amounts of time,
		better simulating day-to-day use.
	*/
	for (size_t i = 0; i < time_proc_mults.size(); i++)
	{
		time_proc_mults[i] = time_mult_dist(mt_rand);
	}

	remove("Log.txt");
	remove("Log.stp");
	std::ofstream olog_file("Log.txt");
	std::ofstream ostp_file("Log.stp");
	ui32 last_r_index = UINT32_MAX;
	ui32 hours_to_log = (ui32)(24 * 365 * 4 * (3600 / (time_div_s / s_to_file_time)));
	for (size_t hours = 0; hours < hours_to_log;)
	{
		if (interrupt_p(mt_rand))
		{
			// Simulate the program being interrupted and started again.
			time_now += interrupt_time(mt_rand);
			ostp_file << time_now / s_to_file_time << ' ' << olog_file.tellp() << log_sep;
		}

		ui32 r_index;
		do
		{
			r_index = index_dist(mt_rand);
		} while (r_index == last_r_index);
		last_r_index = r_index;
		stime_t activ_time = (stime_t)(time_dist(mt_rand) * time_proc_mults[r_index]);

		while (activ_time > 0)
		{
			time_t time_div_now = time_now / time_div_s;
			time_t next_time_div_time = (time_t)(time_div_now + 1) * time_div_s;
			time_t time_div_rem = next_time_div_time - time_now;
			time_t logged_time = min((time_t)activ_time, time_div_rem);
			if ((time_t)activ_time < time_div_rem)
			{
				logged_time = activ_time;
				time_now += logged_time;
			}
			else
			{
				hours++;
				logged_time = time_div_rem;
				time_now += logged_time;
				/*
					After every time division switch, update the time multiplier for each program,
					so that the amount of time an activity tends to take also changes over time.
					This simulates the fact that people don't always use the same programs the same
					amount of times. Over time, peoples interests change and so do their programs.
					NOTE: The multiplicator for idle time is not updated, as idle time would realistically
						tend to stay the same (=> loop starts at 1, not 0).
				*/
				for (size_t m = 1; m < time_proc_mults.size(); m++)
				{
					time_proc_mults[m] = min(time_proc_mults[m] * time_mult_dist(mt_rand), max_time_mult);
				}
				ostp_file << time_now / s_to_file_time << ' ' << olog_file.tellp() << log_sep;
			}
			f32 logged_time_f = (f32)logged_time / (f32)s_to_file_time;
			olog_file << proc_names[r_index] << log_sep << (i32)r_index - 1 << log_sep << '-' << log_sep << logged_time_f << log_sep << log_sep;
			activ_time -= logged_time;
		}
	}
}