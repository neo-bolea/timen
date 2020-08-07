// TODO: Reimplement different EOLs, on a per file basis.
/*
	TODO: Investigate this (bug or expected bevavior?:
		React:  Adding invalid program time ('Unknown' for 0.001550s) to next program ('Hourly Interrupt').
		React:  Adding invalid program time ('Unknown' for 0.100530s) to next program ('P:\C++\timen\build\timen.exe').
*/

// Visual Studio doesn't know what macros are defined in the build file, so dummy macros are defined here (that aren't actually defined).
#ifndef VS_DUMMY
#define DEBUG
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <shellapi.h>
#include <Shlwapi.h>
#include <UIAutomation.h>
#include <AtlBase.h>
#include <AtlCom.h>

#include <atomic>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

typedef float f32;
typedef double f64;

#define internal static
#define global static
#define persist static

#ifdef DEBUG
#define assert(Expr) \
	if(!(Expr))        \
	{                  \
		throw 0;         \
	}                  \
	0
#else
#define assert(Expr) \
	(Expr);            \
	0
#endif
#define UNDEFINED_CODE_PATH assert(false)

#define ArrayLength(Array) (sizeof(Array) / sizeof(Array[0]))
#define PrintError()                                \
	{                                                 \
		char Error[255];                                \
		wsprintf(Error, "ERROR: %i\n", GetLastError()); \
		OutputDebugString(Error);                       \
	}                                                 \
	0

#define MAX(A, B) (((A) < (B)) ? (B) : (A))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

#pragma region String Helpers
internal void
StrECpy(char *Dst, const char *Src, char EndChar)
{
	while(*Src && *Src != EndChar)
	{
		*Dst++ = *Src++;
	}
}

internal ui32
StrCCount(const char *Str, char C)
{
	ui32 Count = 0;
	while(*Str)
	{
		if(*Str++ == C)
		{
			Count++;
		}
	}
	return Count;
}

internal char *
StrEnd(char *Str)
{
	while(*Str)
	{
		Str++;
	}
	return Str;
}

internal char *
StrStrRFind(char *Str, const char *SubStr, size_t StrLen = (size_t)-1, size_t SubStrLen = (size_t)-1)
{
	StrLen = (StrLen == (size_t)-1) ? strlen(Str) : StrLen;
	SubStrLen = (SubStrLen == (size_t)-1) ? strlen(SubStr) : SubStrLen;
	for(char *C = Str + StrLen - SubStrLen; C >= Str; C--)
	{
		if(strncmp(C, SubStr, SubStrLen) == 0)
		{
			return C;
		}
	}
	return 0;
}

internal void
WidenUTF(wchar_t *Dst, i32 DstLen, char *Src, i32 SrcLen = -1)
{
	if(SrcLen == -1)
	{
		SrcLen = (i32)strlen(Src);
	}
	SrcLen++;
	assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Src, SrcLen, Dst, DstLen));
}

internal wchar_t *
WidenUTFAlloc(char *Str, i32 StrLen = -1)
{
	if(StrLen == -1)
	{
		StrLen = (i32)strlen(Str);
	}
	StrLen++;
	i32 ReqSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Str, StrLen, 0, 0);
	wchar_t *Res = (wchar_t *)malloc(ReqSize);
	WidenUTF(Res, ReqSize, Str, StrLen);
	return Res;
}

internal void
NarrowUTF(char *Dst, i32 DstLen, wchar_t *Src, i32 SrcLen = -1)
{
	if(SrcLen == -1)
	{
		SrcLen = (i32)wcslen(Src);
	}
	SrcLen++;
	assert(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Src, SrcLen, Dst, DstLen, 0, 0));
}

internal char *
NarrowUTFAlloc(wchar_t *Str, i32 StrLen = -1)
{
	if(StrLen == -1)
	{
		StrLen = (i32)wcslen(Str);
	}
	StrLen++;
	i32 ReqSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Str, StrLen, 0, 0, 0, 0);
	char *Res = (char *)malloc(ReqSize);
	NarrowUTF(Res, ReqSize, Str, StrLen);
	return Res;
}

#define FreeUTF(Ptr) free((Ptr));
#pragma endregion

#pragma region EOL
typedef enum eol_type
{
	LF,
	CRLF,
	CR,
	EOL_UNDEFINED
} eol_type;

global const eol_type EOLType = LF;
global const char *EOLStr = ",";
global const ui32 EOLLen = (ui32)strlen(EOLStr);

internal bool
IsEOLChar(char C)
{
	for(size_t i = 0; i < EOLLen; i++)
	{
		if(C == EOLStr[i])
		{
			return true;
		}
	}
	return false;
}

// Compares strings until EOL or end of string
// EOLStr is the string that contains the new line character, Str is assumed to have no new line
internal bool
StrCmpEOL(const char *MulLineStr, const char *Str)
{
	while(*MulLineStr == *Str)
	{
		MulLineStr++, Str++;
		if(!*MulLineStr || IsEOLChar(*MulLineStr))
		{
			return true;
		}
	}
	return false;
}

internal ui32
EOLCount(const char *Str)
{
	ui32 Count = 0;
	while(*Str)
	{
		Count += IsEOLChar(*Str++);
	}
	return Count;
}

internal char *
AfterNextEOL(char *Str)
{
	do
	{
		if(*Str == EOLStr[0])
		{
			return Str + EOLLen;
		}
	} while(*Str++);
	return 0;
}

internal char *
InsertEOL(char *Str)
{
	for(size_t i = 0; i < EOLLen; i++)
	{
		*Str++ = EOLStr[i];
	}
	return Str;
}
#pragma endregion

#pragma region Debug Logging
// TODO: Remove event distinction?
enum timen_event
{
	TE_Other = 0,
	TE_Reaction, // Synonymous to logging an activity switch
	TE_Action // Synonymous to switching a program or transitioning into/from idle
};
timen_event LastTimenEvent = TE_Other;

HANDLE DebugConsole;
#ifdef DEBUG
/*
	While this functions acts as a logging function, it's second use is for asserting
	that a given reaction event is always paired with a preceeding action event.
	That means that the program is asserted to run like this:
	Action -> Reaction -> Action -> Reaction -> ...
	Depending on how the program evolves, this naive assertion might be deprecated.
*/
internal void
LogV(timen_event Event, ui8 Color, const char *Format, va_list Args)
{
	printf((Event == TE_Reaction) ? "React:  " : (Event == TE_Reaction) ? "\nAction: " : "\n");
	SetConsoleTextAttribute(DebugConsole, Color);
	vprintf(Format, Args);
	SetConsoleTextAttribute(DebugConsole, 0x07);

	persist timen_event LastEvent = TE_Other;
	//assert(Event != LastEvent);
	LastEvent = Event;
}
#else
internal void
LogV(timen_event Event, ui8 Color, const char *Format, va_list Args)
{
	SetConsoleTextAttribute(DebugConsole, Color);
	vprintf(Format, Args);
	SetConsoleTextAttribute(DebugConsole, LC_Default);
}
#endif

internal void
Log(timen_event Event, const char *Format...)
{
	va_list Args;
	va_start(Args, Format);
	LogV(Event, 0x07, Format, Args);
	va_end(Args);
}

internal void
Log(timen_event Event, ui8 Color, const char *Format...)
{
	va_list Args;
	va_start(Args, Format);
	LogV(Event, Color, Format, Args);
	va_end(Args);
}
#pragma endregion

internal ui32
GetFileSize32(HANDLE File)
{
	LARGE_INTEGER FileSize64;
	assert(GetFileSizeEx(File, &FileSize64));
	assert(FileSize64.QuadPart < UINT32_MAX);
	return (ui32)FileSize64.QuadPart;
}

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
	strcpy_s(NewSym.Str, ArrayLength(NewSym.Str), Str);
	NewSym.ID = (ID == NEW_SYM_ID) ? Syms->GUID++ : ID;
	NewSym.LastActive = Time;

	// Find the oldest symbol, which we will overwrite
	ui64 OldestTime = UINT64_MAX;
	ui32 OldestIndex = 0;
	for(ui32 i = 0; i < ArrayLength(Syms->Symbols); i++)
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

	for(ui32 i = 0; i < ArrayLength(Syms->Symbols); i++)
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

	// THe symbol doesn't exist yet, create it
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
			Log(TE_Reaction, 0x02, "Logging '%s' for %fs\n", Title, fProcTime);
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
		Log(TE_Reaction, 0x04, "Adding invalid program time ('%s' for %fs) to next program ('%s').\n", Title, fProcTime, NextTitle);
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
			if(GetProcessImageFileName(FGProc, ProcBufW, ArrayLength(ProcBufW)))
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

int __stdcall main(HINSTANCE hInstance,
									 HINSTANCE hPrevInstance,
									 LPSTR lpCmdLine,
									 int nShowCmd)
{
#ifdef DEBUG
	DebugConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	DWORD ThreadID;
	HANDLE ThreadHnd = CreateThread(0, 0, NotifIconThread, hInstance, 0, &ThreadID);
	assert(ThreadHnd != INVALID_HANDLE_VALUE);

	const wchar_t *DataDir = L"..\\data";
	CreateDirectory(DataDir, 0);
	assert(SetCurrentDirectory(DataDir));

	wchar_t *LogFilename = L"Log.txt";
	wchar_t *SymFilename = L"Log.sym";
	wchar_t *StampFilename = L"Log.stp";

	proc_sym_table Symbols;
	// By filling with zero we assure that unused symbol slots will be overwritten (since they will have a timestamp of 0, the oldest number)
	memset(&Symbols, 0, sizeof(proc_sym_table));

	if(PathFileExists(LogFilename))
	{
#ifdef DEBUG
		assert(DeleteFile(LogFilename));
#endif
	}
	HANDLE LogFile = CreateFile(LogFilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(LogFile != INVALID_HANDLE_VALUE);

	/*
		The symbol file stores all process names, since they are very repetitive and do not have to be written each time.
	*/
	Symbols.SymFile = CreateFile(SymFilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(Symbols.SymFile != INVALID_HANDLE_VALUE);

	/*
		Time Divisions:
		All activities are grouped into time divisions. If a time division is every hour, that means 
		that all activities in that hour are grouped into that division. That way we can navigate
		the file easily by looking up the time division.

		The stamp file indicates what time division started at which activity (e.g. 4th hour of 8/2/20 started at activity X),
		thus allowing us to navigate the file (and efficiently at that).
	*/
	HANDLE StampFile = CreateFile(StampFilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
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
	const ui32 TimeDivS = 3;
	const ui32 InactivityThreshMS = 2000;

	// Windows Filetime is in 100 nanoseconds intervals
	ui32 MSToFiletime = 10000;
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
	char CurTitle[511] = "\0", ProcessName[MAX_PATH] = "\0";

	// TODO NOW: Set the hour to the last logged file hour
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
				ui32 NewTitleLen = GetWindowText(ActiveWin, NewTitleW, ArrayLength(NewTitleW));

				if(NewTitleLen && GetActiveProgramInfo(ActiveWin, ProcessName, ArrayLength(ProcessName)))
				{
					NarrowUTF(NextTitle, ArrayLength(NextTitle), NewTitleW);
					NextActivityType = AT_Prog;
				}
			}
		}

		if((ActivityType != NextActivityType) || (strcmp(CurTitle, NextTitle) != 0))
		{
			QueryPerformanceCounter(&ProcEnd);
			LARGE_INTEGER NextProcBegin = ProcEnd;

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

			char *CurLogName = GetActivityTitle(ActivityType, CurTitle);
			char *NextLogName = GetActivityTitle(NextActivityType, NextTitle);
			LogProgram(LogFile, Symbols,
								 CurLogName, NextLogName, ProcessName, "-",
								 ActivityType, NextActivityType,
								 ProcBegin, ProcEnd, NextProcBegin, QueryFreq);

			ProcBegin = NextProcBegin;

			Log(TE_Action, "Switching from '%s' to '%s'\n", CurLogName, NextLogName);
			strcpy_s(CurTitle, NextTitle);
		}

		ActivityType = NextActivityType;

		if(Running)
		{
			// TODO: Is it defined behaviour to log two program in one loop pass?
			// Check if the time division switched
			{
				SYSTEMTIME SysTimeNow;
				FILETIME FileTimeNow;
				GetSystemTime(&SysTimeNow);
				SystemTimeToFileTime(&SysTimeNow, &FileTimeNow);
				ui64 CurTimeDiv = ULARGE_INTEGER{FileTimeNow.dwLowDateTime, FileTimeNow.dwHighDateTime}.QuadPart / (TimeDivS * 1000LL * 1000LL * 10LL);

				persist ui64 TimeDivLast = CurTimeDiv;
				/*
					Since we want to know at what activity a time division started, it's easiest if an activity
					starts exactly at that the time division switch. That's why, if an activity goes over from
					one time division to another, we split that activity in two: before and after the division.
				*/
				if(TimeDivLast != CurTimeDiv)
				{
					TimeDivLast = CurTimeDiv;
					Log(TE_Other, 0x0e, "Time division changed: Finishing and logging current activity.\n");

					QueryPerformanceCounter(&ProcEnd);
					LARGE_INTEGER NextProcBegin = ProcEnd;
					char *CurLogName = GetActivityTitle(ActivityType, CurTitle);
					LogProgram(LogFile, Symbols,
										 CurLogName, "Time Division Interrupt", ProcessName, "-",
										 ActivityType, NextActivityType,
										 ProcBegin, ProcEnd, NextProcBegin, QueryFreq);
					ProcBegin = NextProcBegin;

					// Write a stamp to indicate the activity with which the next time division is starting.
					char TimeDivBuf[255];
					LARGE_INTEGER LastActivityEnd;
					assert(GetFileSizeEx(LogFile, &LastActivityEnd));
					ui32 TimeDivLen = sprintf_s(TimeDivBuf, "%llu %llu%s", CurTimeDiv, LastActivityEnd.QuadPart, EOLStr);
					DWORD BytesWritten;
					assert(WriteFile(StampFile, TimeDivBuf, TimeDivLen, &BytesWritten, 0) && BytesWritten == TimeDivLen);
				}
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

	f64 TotalProgramTime = ((f64)(WholeProgramEnd.QuadPart - WholeProgramBegin.QuadPart) / (f64)QueryFreq.QuadPart);
	f64 TimeError = TotalProgramTime - AccumTimeMS;
	assert(TimeError < 0.2f); // TODO: Fix time-precision problems (including ASSURE_TIME_PRECISION())
	assert(!TimeErrorCount);

	assert(WaitForSingleObject(ThreadHnd, 1000) != WAIT_TIMEOUT);

	CloseHandle(LogFile);
	CloseHandle(Symbols.SymFile);

	return 0;
}

typedef struct win_data
{
	NOTIFYICONDATA *NotifIcon;
} win_data;

#define WM_USER_SHELLICON WM_USER + 1
#define IDM_EXIT WM_USER + 2
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
				case IDM_EXIT:
					PostQuitMessage(0);
					break;
				default:
					return DefWindowProc(Wnd, Msg, W, L);
			}
			break;
		}

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

DWORD WINAPI NotifIconThread(void *Args)
{
	HINSTANCE hInstance = (HINSTANCE)Args;

	win_data WD;
	HWND Window;
	NOTIFYICONDATA NotifIcon;
	{
		WNDCLASSEX WndClass;
		memset(&WndClass, 0, sizeof(WNDCLASSEX));
		WndClass.cbSize = sizeof(WNDCLASSEX);
		WndClass.lpfnWndProc = &WndProc;
		WndClass.hInstance = hInstance;
		WndClass.lpszClassName = L"timen";
		WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
		assert(RegisterClassEx(&WndClass));
		Window = CreateWindow(WndClass.lpszClassName, L"timen", WS_MINIMIZE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
		assert(Window != INVALID_HANDLE_VALUE);
		SetWindowLongPtr(Window, GWLP_USERDATA, (LONG_PTR)&WD);

		WD.NotifIcon = &NotifIcon;

		memset(&NotifIcon, 0, sizeof(NOTIFYICONDATA));
		NotifIcon.cbSize = sizeof(NOTIFYICONDATA);
		NotifIcon.hWnd = Window;
		NotifIcon.uID = 0;
		NotifIcon.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		NotifIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		NotifIcon.uCallbackMessage = WM_USER_SHELLICON;
		wcscpy_s(NotifIcon.szTip, ArrayLength(NotifIcon.szTip), L"timen");
		assert(Shell_NotifyIcon(NIM_ADD, &NotifIcon));
	}

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