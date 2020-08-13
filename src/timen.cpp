// TODO: Test program on integrated GPU
/*
	TODO: Add emulation functions (QueryPerformanceCounter, GetActiveProgram, etc.) for faster debugging.
	TODO: Use md5/sha/crc/other for file corruption prevention? (e.g. md5 of each region between two stamps)
	TODO: Save timestamp in other occasions then only when the program starts again, for example when the system shuts down. 
	TODO: Microsoft Edge window title is wrong when queried (weird symbols after 't', then "Edge).
	TODO: Investigate this (bug or expected bevavior?:
		React:  Adding invalid program time ('Unknown' for 0.001550s) to next program ('Hourly Interrupt').
		React:  Adding invalid program time ('Unknown' for 0.100530s) to next program ('P:\C++\timen\build\timen.exe').
*/

// Used for text highlighting, since VS doesn't detect #defines specified in the build file.
#ifndef VS_DUMMY
#define DEBUG
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <Shlwapi.h>
#include <UIAutomation.h>
#include <AtlBase.h>
#include <AtlCom.h>

#include <atomic>
#include <stdio.h>
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

#ifdef DEBUG
#define assert(expr) \
	if(!(expr))        \
	{                  \
		throw 0;         \
	}                  \
	0
#else
#define assert(Expression) (Expression)
#endif
#define UNDEFINED_CODE_PATH assert(false)
// Like UNDEFINED_CODE_PATH, but indicates that the affected code path needs to be worked on to handle the exception properly.
#define UNHANDLED_CODE_PATH assert(false)

#define ArrLen(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define MIN(A, B) ((A) <= (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#include "cc_char.cpp"
#include "cc_io.cpp"

typedef struct process_symbol
{
	char Str[MAX_PATH];
	ui64 LastActive;
	ui32 ID;
} process_symbol;

typedef struct proc_sym_table
{
	process_symbol Symbols[4];
	HANDLE SymFile;
	ui32 GUID;
} proc_sym_table;

internal ui32
FindSymbolInFile(proc_sym_table *Syms, const char *Str)
{
	assert(Syms->SymFile != INVALID_HANDLE_VALUE);
	ui32 FileSize = GetFileSize32(Syms->SymFile);
	if(FileSize)
	{
		char *FileBuf = (char *)VirtualAlloc(0, FileSize + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		assert(FileBuf);

		assert(SetFilePointer(Syms->SymFile, 0, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER);
		DWORD BytesRead;
		assert(ReadFile(Syms->SymFile, FileBuf, FileSize, &BytesRead, 0) && BytesRead == FileSize);
		FileBuf[FileSize] = '\0';

		char *FilePos = FileBuf;
		ui32 ID = 0;
		while(true)
		{
			if(StrCmpEOL(FilePos, Str))
			{
				return ID;
			}

			FilePos = AfterNextEOL(FilePos);
			if(!FilePos)
			{
				break;
			}
			ID++;
		}
		VirtualFree(FileBuf, FileSize, MEM_RELEASE);
	}
	return UINT32_MAX;
}

global const ui32 NEW_SYM_ID = UINT32_MAX;

internal process_symbol
AddSymToTable(proc_sym_table *Syms, const char *Str, ui64 Time, ui32 ID)
{
	// Create the new symbol
	process_symbol NewSym;
	strcpy_s(NewSym.Str, ArrLen(NewSym.Str), Str);
	NewSym.ID = (ID == NEW_SYM_ID) ? Syms->GUID++ : ID;
	NewSym.LastActive = Time;

	// Find the oldest symbol, which we will overwrite
	ui64 OldestTime = UINT64_MAX;
	ui32 OldestIndex = 0;
	for(ui32 i = 0; i < ArrLen(Syms->Symbols); i++)
	{
		ui64 CurTime = Syms->Symbols[i].LastActive;
		if(CurTime < OldestTime)
		{
			OldestTime = CurTime;
			OldestIndex = i;
		}
	}
	Syms->Symbols[OldestIndex] = NewSym;

	return NewSym;
}

internal ui32
ProcessProcSym(proc_sym_table *Syms, const char *Str)
{
	assert(*Str);

	SYSTEMTIME SysTime;
	FILETIME FileTime;
	GetSystemTime(&SysTime);
	assert(SystemTimeToFileTime(&SysTime, &FileTime));
	ui64 Time = (ui64)FileTime.dwLowDateTime | ((ui64)FileTime.dwHighDateTime << 32);

	for(ui32 i = 0; i < ArrLen(Syms->Symbols); i++)
	{
		if(strcmp(Syms->Symbols[i].Str, Str) == 0)
		{
			Syms->Symbols[i].LastActive = Time;
			return Syms->Symbols[i].ID;
		}
	}

	// Try to find the symbol in the symbol file
	ui32 FileID = FindSymbolInFile(Syms, Str);
	if(FileID != UINT32_MAX)
	{
		AddSymToTable(Syms, Str, Time, FileID);
		return FileID;
	}

	// The symbol doesn't exist yet, create it
	{
		process_symbol NewSym = AddSymToTable(Syms, Str, Time, NEW_SYM_ID);

		// Write the new symbol to the file
		DWORD BytesWritten;
		char *SymBufEnd = StrEnd(NewSym.Str);
		SymBufEnd = InsertEOL(SymBufEnd);
		*SymBufEnd = '\0';
		ui32 NewSymBufLen = (ui32)(SymBufEnd - NewSym.Str);
		assert(SetFilePointer(Syms->SymFile, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER);
		assert(WriteFile(Syms->SymFile, NewSym.Str, NewSymBufLen, &BytesWritten, 0));
		assert((i32)BytesWritten == NewSymBufLen);

		return NewSym.ID;
	}
}

// TODO: Remove these variables once time-precision debugging is finished.
global LARGE_INTEGER WholeProgramBegin = {}, WholeProgramEnd = {};
global f64 AccumTimeMS = 0.0f;

// Describes the action that this program has taken in a loop pass
enum activity_type
{
	AT_Invalid = -1,

	AT_Unknown,
	AT_Prog,
	AT_Idle,
};

internal bool
ActivityIsLoggable(activity_type Type)
{
	return Type == AT_Prog || Type == AT_Idle;
}

internal char *
GetActivityTitle(activity_type Type, char *Title)
{
	switch(Type)
	{
		case AT_Prog:
			return Title;
		case AT_Unknown:
			return "Unknown";
		case AT_Idle:
			return "Idle";
		default:
			return "???";
	}
}

internal void
LogProgram(HANDLE File, proc_sym_table &Symbols,
					 char *Title, char *NextTitle, char *ProcessName, const char *Info,
					 activity_type ActivityType, activity_type NextActivityType,
					 LARGE_INTEGER TimeBegin, LARGE_INTEGER TimeEnd, LARGE_INTEGER &NextTimeBegin, LARGE_INTEGER QueryFreq)
{
	// Note: The program time can be negative if the user became idle before the program started, so we need to check for that...
	f64 fProcTime = ((f64)(TimeEnd.QuadPart - TimeBegin.QuadPart) / (f64)QueryFreq.QuadPart);
	if(ActivityIsLoggable(ActivityType) && fProcTime > 0)
	{
		i32 ProcSymValue = -1;
		if(ActivityType != AT_Idle)
		{
			// Convert process name to a symbol
			char *ProcName = strrchr(ProcessName, '\\') + 1;
			ProcSymValue = ProcessProcSym(&Symbols, ProcName);
			assert((i32)FindSymbolInFile(&Symbols, ProcName) == ProcSymValue);
		}

		// TODO: Don't log title if it proves to be unnecessary (especially since titles use up a lot of memory...).
		// TODO: Add extra info to certain windows (e.g. tab/site info for browsers).
		{
			ui64 TitleLen = strlen(Title);
			assert(TitleLen);
			// TODO: Fix this (this was a temp fix).
			char LogBuffer[1024];
			if(TitleLen > 1000)
			{
				Title[1000] = '\0';
			}
			sprintf_s(LogBuffer, "%s%s%i%s%s%s%f%s%s", Title, EOLStr, ProcSymValue, EOLStr, Info, EOLStr, fProcTime, EOLStr, EOLStr);
			Log(0x02, "Logging '%s' for %fs\n", Title, fProcTime);
			i32 LogLen = (i32)strlen(LogBuffer);

			DWORD BytesWritten;
			assert(SetFilePointer(File, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER);
			assert(WriteFile(File, LogBuffer, LogLen, &BytesWritten, 0) && (i32)BytesWritten == LogLen);
		}

#ifdef DEBUG
		// Assert that there are no time leaks (no 'lost' seconds) and that conservation of time is correct.
		// Idle time is an exception, since we might later 'reclaim' time as idle, which would make the assertion temporarily a false positive.
		AccumTimeMS += fProcTime;
		if(NextActivityType != AT_Idle)
		{
			QueryPerformanceCounter(&WholeProgramEnd);
			f64 TotalProgramTime = ((f64)(WholeProgramEnd.QuadPart - WholeProgramBegin.QuadPart) / (f64)QueryFreq.QuadPart);
			f64 TimeError = fabs(TotalProgramTime - AccumTimeMS);
			if(TimeError >= 0.2f)
			{
				printf("TIME ERROR: %f (%f not %f)\n", TimeError, AccumTimeMS, TotalProgramTime);
				UNDEFINED_CODE_PATH;
			}
			else
			{
				printf("Time Precision: %f (%f not %f)\n", TimeError, AccumTimeMS, TotalProgramTime);
			}
		}
#endif
	}
	else if(fProcTime <= 0 || !ActivityIsLoggable(ActivityType))
	{
		// If the current activity is somehow invalid, add the time to the next activity.
		NextTimeBegin.QuadPart -= (ui64)(fProcTime * QueryFreq.QuadPart);
		Log(0x04, "Adding invalid program time ('%s' for %fs) to next program ('%s').\n", Title, fProcTime, NextTitle);
	}
}

internal bool
GetActiveProgramInfo(HWND ActiveWin, char *ProcBuf, ui32 ProcBufLen)
{
	wchar_t ProcBufW[255];
	DWORD ProcID;
	if(GetWindowThreadProcessId(ActiveWin, &ProcID))
	{
		assert(ProcID);
		HANDLE FGProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, ProcID);
		if(FGProc != INVALID_HANDLE_VALUE)
		{
			if(GetProcessImageFileName(FGProc, ProcBufW, ArrLen(ProcBufW)))
			{
				NarrowUTF(ProcBuf, ProcBufLen, ProcBufW);
				return true;
			}
		}
	}
	return false;
}

std::atomic_bool Running = true;
DWORD WINAPI NotifIconThread(void *Args);

// Windows Filetime is in 100 nanoseconds intervals
const ui64 MSToFiletime = 10000;
const ui64 SToFiletime = 10000000;

const char *LogFilename = "Log.txt";
const char *SymFilename = "Log.sym";
const char *StampFilename = "Log.stp";
const wchar_t *LogFilenameW = L"Log.txt";
const wchar_t *SymFilenameW = L"Log.sym";
const wchar_t *StampFilenameW = L"Log.stp";

const ui32 TimeDivS = 10;
void AddStamp(HANDLE LogFile, HANDLE StampFile, ui64 Time)
{
	char TimeDivBuf[255];
	LARGE_INTEGER LastActivityEnd;
	assert(GetFileSizeEx(LogFile, &LastActivityEnd));
	ui32 TimeDivLen = sprintf_s(TimeDivBuf, "%llu %llu%s", Time, LastActivityEnd.QuadPart, EOLStr);
	DWORD BytesWritten;
	assert(WriteFile(StampFile, TimeDivBuf, TimeDivLen, &BytesWritten, 0) && BytesWritten == TimeDivLen);
}


int __stdcall main(HINSTANCE hInstance,
									 HINSTANCE hPrevInstance,
									 LPSTR lpCmdLine,
									 int nShowCmd)
{
	const wchar_t *DataDir = L"..\\data";
	CreateDirectory(DataDir, 0);
	assert(SetCurrentDirectory(DataDir));

#ifdef DEBUG
	DebugConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	DWORD ThreadID;
	HANDLE ThreadHnd = CreateThread(0, 0, NotifIconThread, hInstance, 0, &ThreadID);
	assert(ThreadHnd != INVALID_HANDLE_VALUE);


	WaitForSingleObject(ThreadHnd, INFINITE);
	return 0;

	// By initializing to zero we assure that unused symbol slots will be overwritten (since they will have a timestamp of 0, the oldest number)
	proc_sym_table Symbols = {};

	if(PathFileExists(LogFilenameW))
	{
#ifdef DEBUG
		assert(DeleteFile(LogFilenameW));
#else
		assert(SetFileAttributes(LogFilename, FILE_ATTRIBUTE_NORMAL));
#endif
	}
	HANDLE LogFile = CreateFile(LogFilenameW, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(LogFile != INVALID_HANDLE_VALUE);

	/*
		The symbol file stores all process names, since they are very repetitive and do not have to be written each time.
	*/
	if(PathFileExists(SymFilenameW))
	{
		assert(SetFileAttributes(SymFilenameW, FILE_ATTRIBUTE_NORMAL));
	}
	Symbols.SymFile = CreateFile(SymFilenameW, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(Symbols.SymFile != INVALID_HANDLE_VALUE);

	/*
		Time Divisions:
		All activities are grouped into time divisions. If a time division is every hour, that means 
		that all activities in that hour are grouped into that division. That way we can navigate
		the file easily by looking up the time division.

		The stamp file indicates what time division started at which activity (e.g. 4th hour of 8/2/20 started at activity X),
		thus allowing us to navigate the file (and efficiently at that).
	*/
#ifdef DEBUG
	if(PathFileExists(StampFilenameW))
	{
		assert(DeleteFile(StampFilenameW));
	}
#endif
	HANDLE StampFile = CreateFile(StampFilenameW, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(Symbols.SymFile != INVALID_HANDLE_VALUE);

	// Count the amount of symbols that are currently stored, to correctly set the GUID
	{
		ui32 FileSize = GetFileSize32(Symbols.SymFile);
		if(FileSize)
		{
			char *FileBuf = (char *)VirtualAlloc(0, FileSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			DWORD BytesRead;
			assert(ReadFile(Symbols.SymFile, FileBuf, FileSize, &BytesRead, 0) && BytesRead == FileSize);

			Symbols.GUID = EOLCount(FileBuf);

			VirtualFree(FileBuf, FileSize, MEM_RELEASE);
		}
	}

	// TODO: Switch to second granularity once finished debugging.
	// TODO: Also log seconds, not milliseconds.
	const ui32 SleepMS = 100;
	const ui32 InactivityThreshMS = 20000;

	ui32 Sleep100NS = SleepMS * MSToFiletime;

	FILETIME SysTimeFile;
	GetSystemTimeAsFileTime(&SysTimeFile);
	ULARGE_INTEGER SysTime = ULARGE_INTEGER{SysTimeFile.dwLowDateTime, SysTimeFile.dwHighDateTime};

	// Calculates the time remainder the timer should adhere to. See Sleep() below for further explanations.
	ui64 TimePrecisionRem = SysTime.QuadPart % (Sleep100NS);
	ui64 Iterations = 0, TimeErrorCount = 0, IndivTimeErrorCount = 0;

	LARGE_INTEGER QueryFreq;
	QueryPerformanceFrequency(&QueryFreq);
	LARGE_INTEGER ProcBegin = {}, ProcEnd = {};
	// TODO NOW: Merge title and process to separate program struct (and all next_ and cur_ variables)?
	char CurTitle[511] = "\0", CurProcessName[MAX_PATH] = "\0", NextProcessName[MAX_PATH] = "\0";

	// Add a stamp, since we need to know when activity logging started again.
	{
		SYSTEMTIME SysTimeNow;
		FILETIME FileTimeNow;
		GetSystemTime(&SysTimeNow);
		SystemTimeToFileTime(&SysTimeNow, &FileTimeNow);
		ui64 CurTime = ULARGE_INTEGER{FileTimeNow.dwLowDateTime, FileTimeNow.dwHighDateTime}.QuadPart / SToFiletime;
		AddStamp(LogFile, StampFile, CurTime);
	}

	// TODO NOW: Integrate this into the code
	ULARGE_INTEGER YearBegin2020;
	{
		FILETIME FileTime2020;
		SYSTEMTIME SysTime2020 = {};
		SysTime2020.wYear = 2020;
		SysTime2020.wMonth = 1;
		SysTime2020.wDay = 1;
		SystemTimeToFileTime(&SysTime2020, &FileTime2020);
		YearBegin2020.LowPart = FileTime2020.dwLowDateTime;
		YearBegin2020.HighPart = FileTime2020.dwHighDateTime;
	}

	// TODO NOW: Remove
	SYSTEMTIME SysTimeStart;
	GetSystemTime(&SysTimeStart);

	QueryPerformanceCounter(&ProcBegin);
	WholeProgramBegin = ProcBegin;

	while(Running)
	{
		persist activity_type ActivityType = AT_Invalid;
		activity_type NextActivityType = AT_Invalid;

		persist ui64 IdleTime = 0;
		{
			persist DWORD PrevLastInput = 0;
			// TODO: Add code to detect whether a video is playing, and don't count watching videos as idle time.
			// By not directly using LastInputInfo's time, only comparing to it, we prevent overflow problems for integers.
			LASTINPUTINFO LastInputInfo = {sizeof(LASTINPUTINFO), 0};
			assert(GetLastInputInfo(&LastInputInfo));
			if(PrevLastInput != LastInputInfo.dwTime)
			{
				PrevLastInput = LastInputInfo.dwTime;
				IdleTime = 0;
			}
			IdleTime += SleepMS;

			if(IdleTime >= InactivityThreshMS)
			{
				NextActivityType = AT_Idle;
			}
		}

		char NextTitle[511];
		// If the user isn't idle, check if the program changed
		if(NextActivityType != AT_Idle)
		{
			NextActivityType = AT_Unknown;
			HWND ActiveWin = GetForegroundWindow();
			if(ActiveWin)
			{
				wchar_t NewTitleW[255];
				ui32 NewTitleLen = GetWindowText(ActiveWin, NewTitleW, ArrLen(NewTitleW));

				if(NewTitleLen && GetActiveProgramInfo(ActiveWin, NextProcessName, ArrLen(NextProcessName)))
				{
					NarrowUTF(NextTitle, ArrLen(NextTitle), NewTitleW);
					NextActivityType = AT_Prog;
				}
			}
		}

		char *CurLogName = 0, *NextLogName = 0;
		bool ActivityChange = (ActivityType != NextActivityType) || (strcmp(CurTitle, NextTitle) != 0);
		if(ActivityChange)
		{
			QueryPerformanceCounter(&ProcEnd);
			LARGE_INTEGER NextProcBegin = ProcEnd;

			// TODO: What about when the active program switched multiple times after the user became idle?
			/*
				If the user is becoming idle, we need to 'reclaim' the time that we counted as program time and
				instead add it to the idle time, since we couldn't know if the user was idle until the idling 
				threshold was met.
				Note that the last time the user was idle can be _before_ a program switch. In that case the
				logged time will be negative, which is handled further below.
			*/
			if(NextActivityType == AT_Idle)
			{
				ui64 AddedIdleTime = IdleTime * QueryFreq.QuadPart / 1000;
				ProcEnd.QuadPart -= AddedIdleTime;
				NextProcBegin.QuadPart -= AddedIdleTime;
			}

			CurLogName = GetActivityTitle(ActivityType, CurTitle);
			NextLogName = GetActivityTitle(NextActivityType, NextTitle);
			LogProgram(LogFile, Symbols,
								 CurLogName, NextLogName, CurProcessName, "-",
								 ActivityType, NextActivityType,
								 ProcBegin, ProcEnd, NextProcBegin, QueryFreq);

			ProcBegin = NextProcBegin;
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
				ui64 CurTimeDiv = ULARGE_INTEGER{FileTimeNow.dwLowDateTime, FileTimeNow.dwHighDateTime}.QuadPart / (TimeDivS * SToFiletime);

				persist ui64 TimeDivLast = CurTimeDiv;
				/*
					Since we want to know at what activity a time division started, it's easiest if an activity
					starts exactly at that the time division switch. That's why, if an activity goes over from
					one time division to another, we split that activity in two: before and after the division.
				*/
				if(TimeDivLast != CurTimeDiv)
				{
					TimeDivLast = CurTimeDiv;
					Log(0x0e, "Time division changed: Finishing and logging current activity.\n");

					QueryPerformanceCounter(&ProcEnd);
					LARGE_INTEGER NextProcBegin = ProcEnd;
					CurLogName = GetActivityTitle(ActivityType, CurTitle);
					LogProgram(LogFile, Symbols,
										 CurLogName, "Time Division Interrupt", CurProcessName, "-",
										 ActivityType, NextActivityType,
										 ProcBegin, ProcEnd, NextProcBegin, QueryFreq);
					ProcBegin = NextProcBegin;

					// Write a stamp to indicate the activity with which the next time division is starting.
					AddStamp(LogFile, StampFile, CurTimeDiv * TimeDivS);
				}
			}

			ActivityType = NextActivityType;

			// TODO NOW: Was location change necessary?
			if(ActivityChange)
			{
				Log("Switching from '%s' to '%s'\n", CurLogName, NextLogName);
				strcpy_s(CurTitle, NextTitle);
				strcpy_s(CurProcessName, NextProcessName);
			}

			// Check for timing errors
			{
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
				ULARGE_INTEGER TimeNow{TimeNowFile.dwLowDateTime, TimeNowFile.dwHighDateTime};

				ui64 TimeRem = TimeNow.QuadPart % (Sleep100NS);
				assert(TimeRem < INT64_MAX);
				i32 TimeCorrection = (i32)(((i64)TimeRem - (i64)TimePrecisionRem) / MSToFiletime);
				i64 TotalTimeError = (i64)(TimeNow.QuadPart / MSToFiletime) - ((i64)(SysTime.QuadPart + Iterations++ * SleepMS * MSToFiletime) / MSToFiletime);

				SYSTEMTIME ProfTime;
				FileTimeToSystemTime(&TimeNowFile, &ProfTime);
				//printf("Time: %i:%i.%i, to recover: %ims, error: %ims\n", ProfTime.wMinute, ProfTime.wSecond, ProfTime.wMilliseconds, TimeCorrection, (i32)TotalTimeError);

				persist bool LastWasError = false;
				if(TotalTimeError < SleepMS)
				{
					Sleep(SleepMS - TimeCorrection);
					LastWasError = false;
				}
				else
				{
					if(!LastWasError)
					{
						IndivTimeErrorCount++;
					}

					TimeErrorCount++;
					LastWasError = true;
				}
			}
		}
	}

	// TODO NOW: Remove
	SYSTEMTIME SysTimeEnd;
	GetSystemTime(&SysTimeEnd);

	f64 TotalProgramTime = ((f64)(WholeProgramEnd.QuadPart - WholeProgramBegin.QuadPart) / (f64)QueryFreq.QuadPart);
	f64 TimeError = TotalProgramTime - AccumTimeMS;
	assert(TimeError < 0.2f); // TODO: Fix time-precision problems (including ASSURE_TIME_PRECISION())
	assert(!TimeErrorCount);

	assert(WaitForSingleObject(ThreadHnd, 1000) != WAIT_TIMEOUT);

#ifndef DEBUG
	assert(SetFileAttributes(LogFilename, FILE_ATTRIBUTE_READONLY));
	assert(SetFileAttributes(SymFilename, FILE_ATTRIBUTE_READONLY));
#endif

	CloseHandle(LogFile);
	CloseHandle(Symbols.SymFile);

	return 0;
}

typedef struct time_graph_data
{
	uint VAO, VBO, CalProg;
	uint QuadVAO, FBProg;
} time_graph_data;

typedef struct win_data
{
	NOTIFYICONDATA *NotifIcon;
	COLORREF BGColor;
	HWND TimeGraphWnd;
	time_graph_data TimeGraphData;
} win_data;

typedef struct v2
{
	f32 x, y;
} v2;

void ResizeTimeGraph(HWND Wnd, UINT Msg, WPARAM W, LPARAM L);

void UpdateWindowSize(win_data &WD, HWND Wnd, WPARAM W, LPARAM L)
{
	if(WD.TimeGraphWnd)
	{
		int PosX = 50, PosY = 50;
		int Width = (LOWORD(L) - PosX) / 2, Height = (HIWORD(L) - PosY) / 2;

		if(Width > 0 && Height > 0)
		{
			SetWindowPos(WD.TimeGraphWnd, 0, 0, 0, Width, Height, SWP_NOMOVE);
		}
	}
}

#define WM_USER_SHELLICON WM_USER + 1
#define IDM_EXIT WM_USER + 2
#define IDM_OPEN WM_USER + 3
LRESULT CALLBACK WndProc(HWND Wnd, UINT Msg, WPARAM W, LPARAM L)
{
	LRESULT Result = 0;
	win_data *WD = (win_data *)GetWindowLongPtr(Wnd, GWLP_USERDATA);
	switch(Msg)
	{
		case WM_USER_SHELLICON:
		{
			switch(LOWORD(L))
			{
				case WM_RBUTTONDOWN:
				{
					POINT CursorPos;
					GetCursorPos(&CursorPos);
					HMENU hPopMenu = CreatePopupMenu();
					InsertMenu(hPopMenu, (ui32)-1, MF_BYPOSITION | MF_STRING, IDM_OPEN, L"Open");
					InsertMenu(hPopMenu, (ui32)-1, MF_BYPOSITION | MF_STRING, IDM_EXIT, L"Exit");
					SetForegroundWindow(Wnd);
					TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON, CursorPos.x, CursorPos.y, 0, Wnd, NULL);
					return true;
				}
			}
		}
		break;

		case WM_MENUCOMMAND:
		case WM_COMMAND:
		{
			switch(LOWORD(W))
			{
				case IDM_OPEN:
					ShowWindow(Wnd, true);
					break;
				case IDM_EXIT:
					PostQuitMessage(0);
					break;
				default:
					return DefWindowProc(Wnd, Msg, W, L);
			}
			break;
		}

		case WM_SIZE:
			UpdateWindowSize(*WD, Wnd, W, L);
			break;

		case WM_QUERYENDSESSION:
		{
			// TODO: Implement saving
		}
		break;
		case WM_ENDSESSION:
		{
			// TODO: Implement
		}
		break;

		default:
			Result = DefWindowProc(Wnd, Msg, W, L);
	}

	return Result;
}

LRESULT CALLBACK TimeMainGraphProc(HWND Wnd, UINT Msg, WPARAM W, LPARAM L);

DWORD WINAPI NotifIconThread(void *Args)
{
	// TODO: Use manifest file instead, as msdn suggests (https://docs.microsoft.com/en-us/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process)?
	assert(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));

	HINSTANCE hInstance = (HINSTANCE)Args;

	win_data WD = {};
	HWND Window, TimeMainGraph;
	NOTIFYICONDATA NotifIcon;
	{
		WD.BGColor = RGB(0, 80, 120);

		{
			WNDCLASSEX WndClass = {sizeof(WNDCLASSEX)};
			WndClass.lpfnWndProc = &WndProc;
			WndClass.hInstance = hInstance;
			WndClass.lpszClassName = L"timen";
			WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
			WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
			WndClass.style = CS_OWNDC;
			WndClass.hbrBackground = CreateSolidBrush(WD.BGColor);
			assert(RegisterClassEx(&WndClass));
			Window = CreateWindow(WndClass.lpszClassName, L"timen", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
			assert(Window != INVALID_HANDLE_VALUE);
			SetWindowLongPtr(Window, GWLP_USERDATA, (LONG_PTR)&WD);
		}

		{
			WNDCLASS WndClass = {};
			WndClass.lpfnWndProc = &TimeMainGraphProc;
			WndClass.hInstance = hInstance;
			WndClass.lpszClassName = L"time graph";
			WndClass.style = CS_OWNDC;
			assert(RegisterClass(&WndClass));
			TimeMainGraph = CreateWindow(WndClass.lpszClassName, L"time graph", WS_CHILD | WS_VISIBLE, 50, 50, CW_USEDEFAULT, CW_USEDEFAULT, Window, 0, hInstance, 0);
			assert(TimeMainGraph != INVALID_HANDLE_VALUE);
			SetWindowLongPtr(TimeMainGraph, GWLP_USERDATA, (LONG_PTR)&WD);
			WD.TimeGraphWnd = TimeMainGraph;

			RECT WinRect;
			GetClientRect(Window, &WinRect);
			UpdateWindowSize(WD, Window, SIZE_RESTORED, MAKELPARAM(WinRect.right - WinRect.left, WinRect.bottom - WinRect.top));
		}

		WD.NotifIcon = &NotifIcon;
		NotifIcon = {sizeof(NOTIFYICONDATA)};
		NotifIcon.hWnd = Window;
		NotifIcon.uID = 0;
		NotifIcon.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		NotifIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		NotifIcon.uCallbackMessage = WM_USER_SHELLICON;
		wcscpy_s(NotifIcon.szTip, ArrLen(NotifIcon.szTip), L"timen");
		assert(Shell_NotifyIcon(NIM_ADD, &NotifIcon));
	}

#ifdef DEBUG
	ShowWindow(Window, true);
#endif

	MSG Msg;
	while(GetMessage(&Msg, Window, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &NotifIcon);
	DestroyWindow(Window);

	printf("Exiting...\n");

	Running = false;
	return 0;
}

#include "gl_graphics.cpp"

void InitTimeGraph(time_graph_data &tgd, uint Hours)
{
	//if(VAO)
	//{
	//	glDeleteVertexArrays(1, &VAO);
	//	glDeleteBuffers(1, &VBO);
	//}
	//else
	if(!tgd.VAO)
	{
		CreateProgramFromPaths(tgd.CalProg, "time_prog.vert", "time_prog.frag");
		CreateProgramFromPaths(tgd.FBProg, "framebuffer.vert", "framebuffer.frag");

		{
			glGenVertexArrays(1, &tgd.VAO);
			glGenBuffers(1, &tgd.VBO);

			glBindVertexArray(tgd.VAO);
			{
				glBindBuffer(GL_ARRAY_BUFFER, tgd.VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(v2) * Hours * 2, 0, GL_STATIC_DRAW);
				glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(v2), 0);
				glEnableVertexAttribArray(0);
			}
			glBindVertexArray(0);
		}

		{
			uint QuadVBOs[2];
			glGenVertexArrays(1, &tgd.QuadVAO);
			glGenBuffers(2, QuadVBOs);

			glBindVertexArray(tgd.QuadVAO);
			{
				v2 QuadVerts[6] =
						{
								{-1.0f, -1.0f},
								{1.0f, -1.0f},
								{1.0f, 1.0f},

								{-1.0f, -1.0f},
								{1.0f, 1.0f},
								{-1.0f, 1.0f},
						};
				v2 QuadUVs[6] =
						{
								{0.0f, 0.0f},
								{1.0f, 0.0f},
								{1.0f, 1.0f},

								{0.0f, 0.0f},
								{1.0f, 1.0f},
								{0.0f, 1.0f},
						};

				glBindBuffer(GL_ARRAY_BUFFER, QuadVBOs[0]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVerts), QuadVerts, GL_STATIC_DRAW);
				glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(v2), 0);
				glEnableVertexAttribArray(0);

				glBindBuffer(GL_ARRAY_BUFFER, QuadVBOs[1]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(QuadUVs), QuadUVs, GL_STATIC_DRAW);
				glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(v2), 0);
				glEnableVertexAttribArray(1);
			}
			glBindVertexArray(0);
		}
	}
}

ui64 CreateTimestamp(uint Year, uint Month = 0, uint Day = 0, uint Hour = 0, uint Minute = 0, uint Second = 0)
{
	FILETIME FileTime;
	SYSTEMTIME SysTime = {};
	SysTime.wYear = (WORD)Year;
	SysTime.wMonth = (WORD)Month;
	SysTime.wDay = (WORD)Day;
	SysTime.wHour = (WORD)Hour;
	SysTime.wMinute = (WORD)Minute;
	SysTime.wSecond = (WORD)Second;
	SystemTimeToFileTime(&SysTime, &FileTime);
	return ULARGE_INTEGER{FileTime.dwLowDateTime, FileTime.dwHighDateTime}.QuadPart / SToFiletime;
}

// TODO NOW: Remove
#include <vector>

LRESULT CALLBACK TimeMainGraphProc(HWND Wnd, UINT Msg, WPARAM W, LPARAM L)
{
	LRESULT Result = 0;
	win_data *WD = (win_data *)GetWindowLongPtr(Wnd, GWLP_USERDATA);
	time_graph_data &tgd = WD->TimeGraphData;

	const f32 MinTime = 5.0f;
	const f32 MaxTime = 20.0f;
	const uint ProgCount = 64;
	const f32 MaxAccumTime = MaxTime * ProgCount;
	const uint Hours = 8;
	const uint EntryCount = ProgCount * Hours;
	f32 ProgramValues[EntryCount];

	switch(Msg)
	{
		case WM_CREATE:
		{
			InitOpenGL(GetDC(Wnd));
		}
		break;

		case WM_MOUSEWHEEL:
			RedrawWindow(Wnd, 0, 0, RDW_INVALIDATE);
			break;

		// TODO: Test if DPICHANGED is handled correctly by using multiple monitors.
		// TODO: Follow tutorial on https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows and https://github.com/tringi/win32-dpi
		case WM_DPICHANGED:
		case WM_SIZE:
		{
			// TODO: Fix for all DPIs.
			HMONITOR Monitor = MonitorFromWindow(Wnd, MONITOR_DEFAULTTONEAREST);
			DEVICE_SCALE_FACTOR ScaleFactor;
			GetScaleFactorForMonitor(Monitor, &ScaleFactor);
			f32 DPIScale = ScaleFactor / 100.0f;

			uint Width = (uint)(LOWORD(L) * DPIScale), Height = (uint)(HIWORD(L) * DPIScale);
			UpdateFramebuffers(Width, Height);
			glViewport(0, 0, Width, Height);
			RedrawWindow(Wnd, 0, 0, RDW_INVALIDATE);
			Result = DefWindowProc(Wnd, Msg, W, L);
		}
		break;

		// TODO: Paint live while logging, if the window is active
		// TODO: Refactor to unit test
		// TODO: Go again over this whole mess
		case WM_PAINT:
		{
			if(!tgd.VAO)
			{
				InitTimeGraph(tgd, Hours);
			}

			// TODO NOW: Try with sub-hour precision dates.
			// 8/10/2020 16 14 5   17 55 44
			//ui64 StartDate = CreateTimestamp(2020, 8, 10, 15, 0, 5);
			//ui64 EndDate = CreateTimestamp(2020, 8, 10, 17, 55, 44);

			ui64 StartDate = 13241808709;
			ui64 EndDate = CreateTimestamp(2020, 8, 13, 20, 55, 44);

			typedef struct stamp_info
			{
				// The marker which points to an activity in the main log file.
				ui64 Marker;
				// The time at which that activity started.
				ui64 Time;
			} stamp_info;

			// Find the range of the file where the activities to collect are.
			// TODO NOW: Some type of read/write protection or other mechanism, since main thread already writes to the file.
			bool FoundStart = false, FoundEnd = false;
			ui64 StpFileSize;
			char *StpBuf = (char *)LoadFileContents(StampFilename, &StpFileSize);
			// Includes all timestamps in the requested range, but also the two timestamps before and after.
			std::vector<stamp_info> InclDateStamps;
			{
				assert(StartDate < EndDate);
				char *FilePos = StpBuf;

				ui64 LastStpDate = 0;
				ui64 LastStpMarker = 0;
				while((ui64)(FilePos - StpBuf) < StpFileSize)
				{
					ui64 StpDate = strtoull(FilePos, &FilePos, 10);
					char *StpMarkerStr = strchr(FilePos, ' ') + 1;
					ui64 StpMarker = strtoull(StpMarkerStr, 0, 10);
					if(!FoundStart)
					{
						if(StpDate > StartDate)
						{
							assert(!LastStpDate || LastStpDate <= StartDate);
							FoundStart = true;
							FilePos++;

							if(LastStpDate)
							{
								InclDateStamps.push_back({LastStpMarker, LastStpDate});
							}
							else
							{
								InclDateStamps.push_back({StpMarker, StpDate});
							}
						}
					}

					if(FoundStart)
					{
						InclDateStamps.push_back({StpMarker, StpDate});

						if(StpDate >= EndDate)
						{
							if(AfterNextEOL(StpMarkerStr))
							{
								EndDate = StpDate;
							}
							else
							{
								InclDateStamps.push_back({StpFileSize, EndDate});
							}
							FoundEnd = true;
							break;
						}
					}

					FilePos = AfterNextEOL(StpMarkerStr);
					LastStpDate = StpDate;
					LastStpMarker = StpMarker;
				}

				if(!FoundStart)
				{
					assert(InclDateStamps.size() == 0);
				}

				if(!FoundEnd)
				{
					// TODO NOW: Implement
				}
			}

			// Process the activities to make presentable data out of them.
			// TODO NOW: Make vector as long as the amount of created processes, instead of ProgCount
			std::vector<f64> ActivityTimes(ProgCount * Hours);
			if(InclDateStamps.size())
			{
				// There must be at least two timestamps, since otherwise this doesn't represent a range.
				assert(InclDateStamps.size() >= 2);

				// Start date must be before the second stamp, since the stamp is included in the range.
				assert(StartDate <= InclDateStamps[1].Time);
				// Same as above, but for end date.
				assert((InclDateStamps.end() - 2)->Time <= EndDate);

				ui64 StartMarker = InclDateStamps.front().Marker, EndMarker = InclDateStamps.back().Marker;
				// TODO NOW:	set endmarker to end of file if last marker in range is the last one in stp file
				ui64 StartStpTime = InclDateStamps.front().Time;
				ui32 FileRangeSize = (ui32)(EndMarker - StartMarker);
				void *LogFile = OpenFile(LogFilename, FA_Read, FS_All);
				LARGE_INTEGER FilePointer;
				FilePointer.QuadPart = StartMarker;
				assert(SetFilePointerEx(LogFile, FilePointer, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER);
				file_result FileResult;
				char *LogBuf = (char *)ReadFileContents(LogFile, FileRangeSize, &FileResult);
				assert(FileResult == file_result::Success);

				char *LogPos = LogBuf;
				i32 CurStp = 0;
				stamp_info NextStpInfo = InclDateStamps[CurStp + 1];
				ui32 CurHour = 0;

				bool StartCounting = false;
				i64 TimeUntilStartDate = (StartDate - InclDateStamps.front().Time);
				i64 TimeUntilEndDate = (InclDateStamps.back().Time - EndDate);

				while(LogPos >= LogBuf && ((ptrdiff_t)LogPos - (ptrdiff_t)LogBuf) < FileRangeSize)
				{
					while(NextStpInfo.Marker <= (StartMarker + (LogPos - LogBuf)))
					{
						CurHour = (ui32)((NextStpInfo.Time) / TimeDivS - StartStpTime / TimeDivS);
						assert(CurHour <= 7);
						CurStp++;
						if(CurStp == InclDateStamps.size() - 1)
						{
							break;
						}
						NextStpInfo = InclDateStamps[CurStp + 1];
					}

					LogPos = AfterNextEOL(LogPos);
					assert(LogPos);
					i32 AIndex = atoi(LogPos);
					// TODO NOW: Replace with actual amount of processes
					LogPos = AfterNextEOL(LogPos);
					char *Indi = AfterNextEOL(LogPos);
					LogPos = Indi;
					f32 ATimeF = (f32)atof(LogPos);
					ui64 ATime = (ui64)(ATimeF * SToFiletime);
					// assert(ATime > MinProgramTime);
					LogPos = AfterNextEOL(LogPos);
					LogPos = AfterNextEOL(LogPos);

					if(!StartCounting)
					{
						TimeUntilStartDate -= ATime;
						if(TimeUntilStartDate <= 0)
						{
							StartCounting = true;
						}
					}

					if(StartCounting)
					{
						if(AIndex + 1 < ProgCount)
						{
							// TODO FIX: Many(all?) symbol entries are ignored and new ones are added. Many programs are counted as one, for some reason
							uint AHIndex = (AIndex + 1) * Hours + CurHour;
							ActivityTimes[AHIndex] += ATimeF;
							// TODO FIX: Activity time is sometimes larger than division time (precision problem) [remove "+ 1.0f" after fix].
							assert(ActivityTimes[AHIndex] <= TimeDivS + 1.0f);
						}
					}
				}

				FreeFileContents(LogBuf);
			}
			FreeFileContents(StpBuf);

			// Find the largest spike in the graph for normalization.
			f64 LargestTime = 0;
			for(size_t h = 0; h < Hours; h++)
			{
				f64 AccumTime = 0;
				for(size_t p = 0; p < ProgCount; p++)
				{
					AccumTime += ActivityTimes[p * Hours + h];
				}
				LargestTime = max(LargestTime, AccumTime);
			}

			const f32 GraphPaddingMult = 1.1f;
			LargestTime *= GraphPaddingMult;

			for(size_t h = 0; h < Hours; h++)
			{
				for(size_t p = 0; p < ProgCount; p++)
				{
					size_t Index = h * ProgCount + p;
					ProgramValues[Index] = (f32)(ActivityTimes[p * Hours + h] / LargestTime * 2.0f - 1.0f);
					if(p != 0)
					{
						ProgramValues[Index] += ProgramValues[Index - 1] + 1.0f;
					}
				}
			}


			PAINTSTRUCT PS;
			HDC DC = BeginPaint(Wnd, &PS);

			glBindFramebuffer(GL_FRAMEBUFFER, MSFrameBuf);
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUseProgram(tgd.CalProg);
			glBindVertexArray(tgd.VAO);

			for(size_t p = 0; p < ProgCount; p++)
			{
				// TODO: Space out all colors, also space them out from background and special colors (like black, magenta?).
				// TODO: If an 'Other' section is added, make it light gray or some boring color.
				f32 Color[3];
				for(size_t i = 0; i < 3; i++)
				{
					Color[i] = (f32)rand() / RAND_MAX;
				}
				glUniform3fv(glGetUniformLocation(tgd.CalProg, "uColor"), 1, Color);

				v2 Points[Hours * 2] = {};
				for(size_t h = 0; h < Hours; h++)
				{
					f32 x = ((f32)h / (Hours - 1)) * 2.0f - 1.0f;
					if(p != 0)
					{
						Points[2 * h] = {x, ProgramValues[h * ProgCount + p - 1]};
					}
					else
					{
						Points[2 * h] = {x, -1.0f};
					}
					Points[2 * h + 1] = {x, ProgramValues[h * ProgCount + p]};
				}

				glBindBuffer(GL_ARRAY_BUFFER, tgd.VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(v2) * Hours * 2, Points, GL_STATIC_DRAW);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, Hours * 2);
			}

			RECT ClientRect;
			GetClientRect(Wnd, &ClientRect);
			uint ScreenWidth = ClientRect.right - ClientRect.left;
			uint ScreenHeight = ClientRect.bottom - ClientRect.top;
			glBindFramebuffer(GL_READ_FRAMEBUFFER, MSFrameBuf);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, IntermFrameBuf);
			glBlitFramebuffer(0, 0, ScreenWidth, ScreenHeight, 0, 0, ScreenWidth, ScreenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(tgd.FBProg);
			glBindVertexArray(tgd.QuadVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, IntermTex);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			SwapBuffers(DC);

			EndPaint(Wnd, &PS);
			return 0;
		}

		default:
			Result = DefWindowProc(Wnd, Msg, W, L);
	}

	return Result;
}
