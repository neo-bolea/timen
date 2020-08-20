// TODO: Test program on integrated GPU
/*
	TODO: Calendar marks
	TODO REFACTOR: Remove all 'Proc' names

	TODO REFACTOR: Change all ambiguous integers to typedefs (e.g. filetime vs second timestamps)
	TODO: Add emulation functions (QueryPerformanceCounter, GetActiveProgram, etc.) for faster debugging.
	TODO: Use md5/sha/crc/other for file corruption prevention? (e.g. md5 of each region between two stamps)
	TODO: Save timestamp in other occasions then only when the program starts again, for example when the system shuts down. 
	TODO: Microsoft Edge window title is wrong when queried (weird symbols after 't', then "Edge).
	TODO: Investigate this (bug or expected bevavior?: (old?)
		React:  Adding invalid program time ('Unknown' for 0.001550s) to next program ('Hourly Interrupt').
		React:  Adding invalid program time ('Unknown' for 0.100530s) to next program ('P:\C++\timen\build\timen.exe').
*/

// Used for text highlighting, since VS doesn't detect #defines specified in the build file.
#ifndef VS_DUMMY
#define DEBUG
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <Shlwapi.h>
#include <UIAutomation.h>
#include <AtlBase.h>
#include <AtlCom.h>

#include <atomic>
#include <cwchar>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <stdint.h>

typedef unsigned int uint;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;
typedef uint32_t uint;

typedef float f32;
typedef double f64;

#define internal static
#define global static
#define persist static

// TODO FUTURE: Change all NoAsserts to Asserts.
#define NoAssert(Expression) (Expression)
#ifdef DEBUG
// TODO: Convert some assertions to UNHANDLED_CODE_PATHs.
// NOTE: _Only_ use if an assertion is expected to fail at the moment and a fix is planned later.
#define Assert(expr) \
	if(!(expr))        \
	{                  \
		throw 0;         \
	}                  \
	0
#else
#define Assert(Expression) NoAssert(Expression)
#endif
#define UNDEFINED_CODE_PATH Assert(false)
// Like UNDEFINED_CODE_PATH, but indicates that the affected code path needs to be worked on to handle the exception properly.
#define UNHANDLED_CODE_PATH Assert(false)

#define ArrLen(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define MIN(A, B) ((A) <= (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(V, min, max) (MAX(MIN((V), max), min))

#ifdef DEBUG
#define TruncAss(Type, n) (((n) == (decltype(n))(Type)(n)) ? 0 : (throw 0), (Type)(n))
#else
#define TruncAss(Type, n) ((Type)(n))
#endif

typedef struct v2
{
	f32 x, y;
} v2;

#include "cc_char.h"
#include "cc_io.h"

#define ParseLogTime(...) atof(__VA_ARGS__)
#define P_TMIN PRIi64
#define TMIN_MAX INT32_MAX

typedef i64 t_min; // Minimum time unit (highest granularity)
typedef i64 t_min_d;
typedef i64 t_div; // Time division (e.g. hour)
typedef i64 t_div_d;
typedef ui64 t_file; // Used by win32

// Windows FileTime is in 100 nanoseconds intervals
const t_min TMinToTFile = 10000000;

t_file GetYear2020();
typedef struct timen_cfg
{
	const char *LogFilename = "Log.txt";
	const wchar_t *LogFilenameW = L"Log.txt";
	const char *StampFilename = "Log.stp";
	const wchar_t *StampFilenameW = L"Log.stp";
	const char *SymFilename = "Log.sym";
	const wchar_t *SymFilenameW = L"Log.sym";

	t_min SleepTime = 1;
	t_min InactivityThresh = TMIN_MAX;
	t_file GlobalTimeBegin = GetYear2020();
	t_min DivTime = 100;
};

#include "stamps.h"
#include "proc_sym.h"
#include "activities.h"

std::atomic_bool Running = true;

t_file GetYear2020()
{
	LARGE_INTEGER YearBegin2020L;
	FILETIME FileTime2020;
	SYSTEMTIME SysTime2020 = {};
	SysTime2020.wYear = 2020;
	SysTime2020.wMonth = 1;
	SysTime2020.wDay = 1;
	SystemTimeToFileTime(&SysTime2020, &FileTime2020);
	YearBegin2020L.LowPart = FileTime2020.dwLowDateTime;
	YearBegin2020L.HighPart = FileTime2020.dwHighDateTime;
	return YearBegin2020L.QuadPart;
}

#define DO_LOG 1
#define DO_GRAPH 2

#define WHAT_DO DO_GRAPH

#include "ui.h"

int __stdcall main(HINSTANCE hInstance,
									 HINSTANCE hPrevInstance,
									 LPSTR lpCmdLine,
									 int nShowCmd)
{
	const wchar_t *DataDir = L"..\\data";
	CreateDirectory(DataDir, 0);
	Assert(SetCurrentDirectory(DataDir));

#ifdef DEBUG
	DebugConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	CoInitialize(0);

	timen_cfg tcfg;

#if WHAT_DO == DO_GRAPH
	DWORD ThreadID;
	HANDLE ThreadHnd = CreateThread(0, 0, MainWindow, &tcfg, 0, &ThreadID);
	Assert(ThreadHnd != INVALID_HANDLE_VALUE);

	WaitForSingleObject(ThreadHnd, INFINITE);
	return 0;
#endif

	// By initializing to zero we assure that unused symbol slots will be overwritten (since they will have a timestamp of 0, the oldest number)
	process_symbol_table Symbols = {};

	if(PathFileExists(tcfg.LogFilenameW))
	{
#ifdef DEBUG
		//Assert(DeleteFile(LogFilenameW));
#else
		Assert(SetFileAttributes(LogFilename, FILE_ATTRIBUTE_NORMAL));
#endif
	}
	HANDLE LogFile = CreateFile(tcfg.LogFilenameW, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	Assert(LogFile != INVALID_HANDLE_VALUE);

	/*
		The symbol file stores all process names, since they are very repetitive and do not have to be written each time.
	*/
	if(PathFileExists(tcfg.SymFilenameW))
	{
		Assert(SetFileAttributes(tcfg.SymFilenameW, FILE_ATTRIBUTE_NORMAL));
	}
	Symbols.SymFile = CreateFile(tcfg.SymFilenameW, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	Assert(Symbols.SymFile != INVALID_HANDLE_VALUE);

	/*
		Time Divisions:
		All activities are grouped into time divisions. If a time division is every hour, that means 
		that all activities in that hour are grouped into that division. That way we can navigate
		the file easily by looking up the time division.

		The stamp file indicates what time division started at which activity (e.g. 4th hour of 8/2/20 started at activity X),
		thus allowing us to navigate the file (and efficiently at that).
	*/
#ifdef DEBUG
	if(PathFileExists(tcfg.StampFilenameW))
	{
		//Assert(DeleteFile(StampFilenameW));
	}
#endif
	HANDLE StampFile = CreateFile(tcfg.StampFilenameW, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	Assert(Symbols.SymFile != INVALID_HANDLE_VALUE);

	// Count the amount of symbols that are currently stored, to correctly set the GUID
	{
		ui32 FileSize = GetFileSize32(Symbols.SymFile);
		if(FileSize)
		{
			char *FileBuf = (char *)VirtualAlloc(0, FileSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			DWORD BytesRead;
			Assert(ReadFile(Symbols.SymFile, FileBuf, FileSize, &BytesRead, 0) && BytesRead == FileSize);

			Symbols.GUID = EOLCount(FileBuf);

			VirtualFree(FileBuf, FileSize, MEM_RELEASE);
		}
	}

	FILETIME SysTimeFile;
	GetSystemTimeAsFileTime(&SysTimeFile);
	ULARGE_INTEGER SysTime = ULARGE_INTEGER{SysTimeFile.dwLowDateTime, SysTimeFile.dwHighDateTime};

	// Calculates the time remainder the timer should adhere to. See Sleep() below for further explanations.
	t_min Sleep100NS = tcfg.SleepTime * TMinToTFile;
	t_min TimePrecisionRem = SysTime.QuadPart % (Sleep100NS);
	ui64 LogIters = 0;

	t_file QueryFreq;
	QueryPerformanceFrequency((LARGE_INTEGER *)&QueryFreq);
	t_file ActBegin = {}, ActEnd = {};

	// Add a stamp, since we need to know when activity logging started again.
	{
		SYSTEMTIME SysTimeNow;
		FILETIME FileTimeNow;
		GetSystemTime(&SysTimeNow);
		SystemTimeToFileTime(&SysTimeNow, &FileTimeNow);
		t_min CurTime = (ULARGE_INTEGER{FileTimeNow.dwLowDateTime, FileTimeNow.dwHighDateTime}.QuadPart - tcfg.GlobalTimeBegin) / TMinToTFile;
		AddStamp(tcfg, LogFile, StampFile, CurTime);
	}

	QueryPerformanceCounter((LARGE_INTEGER *)&ActBegin);
	WholeProgramBegin = ActBegin;

	activity_info CurA;
	while(Running)
	{
		activity_info NextA = {};
		persist t_min IdleTime = 0;
		persist DWORD PrevLastInput = 0;
		GetNextActivity(IdleTime, PrevLastInput, tcfg.SleepTime, tcfg.InactivityThresh, NextA);

		if(CurA != NextA)
		{
			QueryPerformanceCounter((LARGE_INTEGER *)&ActEnd);
			t_file NextProcBegin = ActEnd;

			// TODO: What about when the active program switched multiple times after the user became idle?
			/*
				If the user is becoming idle, we need to 'reclaim' the time that we counted as program time and
				instead add it to the idle time, since we couldn't know if the user was idle until the idling 
				threshold was met.
				Note that the last time the user was idle can be _before_ a program switch. In that case the
				logged time will be negative, which is handled further below.
			*/
			if(NextA.Type == AT_Idle)
			{
				t_min AddedIdleTime = IdleTime * QueryFreq;
				ActEnd -= AddedIdleTime;
				NextProcBegin -= AddedIdleTime;
			}

			LogActivity(LogFile, Symbols,
									GetActivityTitle(CurA), GetActivityTitle(NextA), CurA.ProcessName, "-",
									CurA.Type, NextA.Type,
									ActBegin, ActEnd, NextProcBegin, QueryFreq);

			ActBegin = NextProcBegin;
		}

		if(Running)
		{
			// TODO: Is it defined behaviour to log two program in one loop pass?
			// Check if the time division switched, and add a stamp if so.
			{
				SYSTEMTIME SysTimeNow;
				FILETIME FileTimeNow;
				GetSystemTime(&SysTimeNow);
				SystemTimeToFileTime(&SysTimeNow, &FileTimeNow);
				t_file TimeRel2020 = ULARGE_INTEGER{FileTimeNow.dwLowDateTime, FileTimeNow.dwHighDateTime}.QuadPart - tcfg.GlobalTimeBegin;
				TimeRel2020 /= TMinToTFile;
				t_div CurTimeDiv = TimeRel2020 / tcfg.DivTime;

				persist t_div TimeDivLast = CurTimeDiv;
				/*
					Since we want to know at what activity a time division started, it's easiest if an activity
					starts exactly at that the time division switch. That's why, if an activity goes over from
					one time division to another, we split that activity in two: before and after the division.
				*/
				if(TimeDivLast != CurTimeDiv)
				{
					TimeDivLast = CurTimeDiv;
					Log(0x0e, "Time division changed: Finishing and logging current activity.\n");

					char InfoBuf[255], TimeBuf[255];
#ifdef DEBUG
					StampToTimeStr(tcfg, TimeBuf, ArrLen(TimeBuf), TimeRel2020);
					sprintf_s(InfoBuf, "Time Switch [%s]", TimeBuf);
#else
					strcpy(InfoBuf, "Time Switch");
#endif

					QueryPerformanceCounter((LARGE_INTEGER *)&ActEnd);
					t_file NextActBegin = ActEnd;
					LogActivity(LogFile, Symbols,
											GetActivityTitle(CurA), "Time Division Interrupt", CurA.ProcessName, InfoBuf,
											CurA.Type, NextA.Type,
											ActBegin, ActEnd, NextActBegin, QueryFreq);
					ActBegin = NextActBegin;

					// Write a stamp to indicate the activity with which the next time division is starting.
					AddStamp(tcfg, LogFile, StampFile, CurTimeDiv * tcfg.DivTime);
				}
			}

			CurA = NextA;

			// Check for timing errors
			{
#ifdef DEBUG
				{
					QueryPerformanceCounter((LARGE_INTEGER *)&ActEnd);
					Assert(ActBegin < ActEnd);
					t_min TimeSinceActSwitch = (ActEnd - ActBegin) / QueryFreq;
					TimeSinceActSwitch /= TMinToTFile;
					Assert(TimeSinceActSwitch <= tcfg.DivTime);
				}
#endif

				/*
					Sleep() doesn't guarantee perfect time precision, and these imprecisions can stack up to very
					high numbers over time. Here we try to manually correct these imprecisions.
					Example: If the sleep interval is one second long, we try to keep the intervals at exactly one second.
						We do that by trying to keep the remainder of the second right (i.e. milliseconds here):
						If the milliseconds for the first iteration are  .517s, we try to continue that pattern:
						1.517s, 2.517s, etc. If the remainder is .520s, we sleep 3ms less.
					This makes the logging more predictable, especially because an hour of activities might be logged as longer
					than an hour, which could be erroneous with the time division navigation.
				*/
				FILETIME TimeNowFile;
				GetSystemTimeAsFileTime(&TimeNowFile);
				t_file TimeNow = ULARGE_INTEGER{TimeNowFile.dwLowDateTime, TimeNowFile.dwHighDateTime}.QuadPart;

				t_file TimeRem = TimeNow % Sleep100NS;
				Assert(TimeRem < TMIN_MAX);
				t_min_d TimeCorrection = ((t_min_d)(TimeRem - TimePrecisionRem) / TMinToTFile);
				t_min_d TotalTimeError = (t_min_d)(TimeNow / TMinToTFile) - ((SysTime.QuadPart + LogIters++ * tcfg.SleepTime * TMinToTFile) / TMinToTFile);

				persist bool LastWasError = false;
				if(TotalTimeError < tcfg.SleepTime)
				{
					Assert(abs(TimeCorrection) <= tcfg.SleepTime);
					Sleep(TruncAss(DWORD, tcfg.SleepTime - TimeCorrection) * 1000);
					LastWasError = false;
				}
				else
				{
					// TODO FIX: Stop assertion from firing
					//UNHANDLED_CODE_PATH;
					int x = 0;
					x = x;
				}
			}
		}
	}

	t_min TotalProgramTime = ((t_min)(WholeProgramEnd - WholeProgramBegin) / (t_min)QueryFreq);
	t_min_d TimeError = TotalProgramTime - AccumTimeS;
	Assert(abs(TimeError) < 1); // TODO: Fix time-precision problems (including ASSURE_TIME_PRECISION())
	Assert(TimeErrorCount == 0);

#if WHAT_DO == DO_GRAPH
	Assert(WaitForSingleObject(ThreadHnd, 1000) != WAIT_TIMEOUT);
#endif

#ifndef DEBUG
	Assert(SetFileAttributes(LogFilename, FILE_ATTRIBUTE_READONLY));
	Assert(SetFileAttributes(SymFilename, FILE_ATTRIBUTE_READONLY));
#endif

	CloseHandle(LogFile);
	CloseHandle(Symbols.SymFile);

	CoUninitialize();

	return 0;
}