// TODO: Reimplement different EOLs, on a per file basis.

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
void StrECpy(char *Dst, const char *Src, char EndChar)
{
	while(*Src && *Src != EndChar)
	{
		*Dst++ = *Src++;
	}
}

ui32 StrCCount(const char *Str, char C)
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

char *StrEnd(char *Str)
{
	while(*Str)
	{
		Str++;
	}
	return Str;
}

char *StrStrRFind(char *Str, const char *SubStr, size_t StrLen = (size_t)-1, size_t SubStrLen = (size_t)-1)
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

void WidenUTF(wchar_t *Dst, i32 DstLen, char *Src, i32 SrcLen = -1)
{
	if(SrcLen == -1)
	{
		SrcLen = (i32)strlen(Src);
	}
	SrcLen++;
	assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Src, SrcLen, Dst, DstLen));
}

wchar_t *WidenUTFAlloc(char *Str, i32 StrLen = -1)
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

void NarrowUTF(char *Dst, i32 DstLen, wchar_t *Src, i32 SrcLen = -1)
{
	if(SrcLen == -1)
	{
		SrcLen = (i32)wcslen(Src);
	}
	SrcLen++;
	assert(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Src, SrcLen, Dst, DstLen, 0, 0));
}

char *NarrowUTFAlloc(wchar_t *Str, i32 StrLen = -1)
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
global const char *EOLStr = "\n";
global const ui32 EOLLen = 1;

bool IsEOLChar(char C)
{
	return C == '\n';
}

// Compares strings until EOL or end of string
// EOLStr is the string that contains the new line character, Str is assumed to have no new line
bool StrCmpEOL(const char *MulLineStr, const char *Str)
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

ui32 EOLCount(const char *Str)
{
	ui32 Count = 0;
	while(*Str)
	{
		Count += IsEOLChar(*Str++);
	}
	return Count;
}

char *AfterNextEOL(char *Str)
{
	do
	{
		if(*Str == '\n')
		{
			return Str + 1;
		}
	} while(*Str++);
	return 0;
}

char *InsertEOL(char *Str)
{
	*Str++ = '\n';
	return Str;
}
#pragma endregion

#pragma region Debug Logging
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
void LogV(timen_event Event, ui8 Color, const char *Format, va_list Args)
{
	printf((Event == TE_Reaction) ? "React:  " : (Event == TE_Reaction) ? "\nAction: " : "\n");
	SetConsoleTextAttribute(DebugConsole, Color);
	vprintf(Format, Args);
	SetConsoleTextAttribute(DebugConsole, 0x07);

	persist timen_event LastEvent = TE_Other;
	assert(Event != LastEvent);
	LastEvent = Event;
}
#else
void LogV(timen_event Event, ui8 Color, const char *Format, va_list Args)
{
	SetConsoleTextAttribute(DebugConsole, Color);
	vprintf(Format, Args);
	SetConsoleTextAttribute(DebugConsole, LC_Default);
}
#endif

void Log(timen_event Event, const char *Format...)
{
	va_list Args;
	va_start(Args, Format);
	LogV(Event, 0x07, Format, Args);
	va_end(Args);
}

void Log(timen_event Event, ui8 Color, const char *Format...)
{
	va_list Args;
	va_start(Args, Format);
	LogV(Event, Color, Format, Args);
	va_end(Args);
}
#pragma endregion

ui32 GetFileSize32(HANDLE File)
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

ui32 FindSymbolInFile(proc_sym_table *Syms, const char *Str)
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
process_symbol AddSymToTable(proc_sym_table *Syms, const char *Str, ui64 Time, ui32 ID)
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

ui32 ProcessProcSym(proc_sym_table *Syms, const char *Str)
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

void LogProgram(HANDLE File, char *Title, i32 ProcSymbol, const char *Info, f64 fProcTime)
{
	ui64 TitleLen = strlen(Title);
	assert(TitleLen);
	// TODO: Fix this (this was a temp fix).
	char LogBuffer[1024];
	if(TitleLen > 1000)
	{
		Title[1000] = '\0';
	}
	sprintf_s(LogBuffer, "%s%s%i%s%s%s%f%s%s", Title, EOLStr, ProcSymbol, EOLStr, Info, EOLStr, fProcTime, EOLStr, EOLStr); // TODO: Log at sub-second precision?
	Log(TE_Reaction, 0x02, "Logging '%s' for %fs\n", Title, fProcTime);
	i32 LogLen = (i32)strlen(LogBuffer);

	DWORD BytesWritten;
	assert(SetFilePointer(File, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER);
	assert(WriteFile(File, LogBuffer, LogLen, &BytesWritten, 0) && (i32)BytesWritten == LogLen);
}

bool GetActiveProgramInfo(HWND ActiveWin, char *ProcBuf, ui32 ProcBufLen)
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

void GetActiveTab(HWND Wnd)
{
	CoInitialize(NULL);
	while(true)
	{
		CComQIPtr<IUIAutomation> uia;
		if(FAILED(uia.CoCreateInstance(CLSID_CUIAutomation)) || !uia)
			return;

		CComPtr<IUIAutomationElement> root;
		if(FAILED(uia->ElementFromHandle(Wnd, &root)) || !root)
			return;

		CComPtr<IUIAutomationCondition> condition;

		//URL's id is 0xC354, or use UIA_EditControlTypeId for 1st edit box
		uia->CreatePropertyCondition(UIA_ControlTypePropertyId,
																 CComVariant(0xC354), &condition);

		//or use edit control's name instead
		//uia->CreatePropertyCondition(UIA_NamePropertyId,
		//      CComVariant(L"Address and search bar"), &condition);

		CComPtr<IUIAutomationElement> edit;
		if(FAILED(root->FindFirst(TreeScope_Descendants, condition, &edit)) || !edit)
			return; //maybe we don't have the right tab, continue...

		CComVariant url;
		edit->GetCurrentPropertyValue(UIA_ValueValuePropertyId, &url);
		MessageBox(0, url.bstrVal, 0, 0);
	}
	CoUninitialize();
}

// TODO: Remove this macro once time-precision debugging is finished.
#ifdef DEBUG
global f64 AccumTimeMS = 0.0f;
#endif

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

	proc_sym_table Symbols;
	// By filling with zero we assure that unused symbol slots will be overwritten (since they will have a timestamp of 0, the oldest number)
	memset(&Symbols, 0, sizeof(proc_sym_table));

	if(PathFileExists(LogFilename))
	{
#ifdef DEBUG
		assert(DeleteFile(LogFilename));
#else
		assert(SetFileAttributes(LogFilename, FILE_ATTRIBUTE_NORMAL));
#endif
	}
	HANDLE LogFile = CreateFile(LogFilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(LogFile != INVALID_HANDLE_VALUE);

	if(PathFileExists(SymFilename))
	{
		assert(SetFileAttributes(SymFilename, FILE_ATTRIBUTE_NORMAL));
	}
	Symbols.SymFile = CreateFile(SymFilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
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
	const ui32 InactivityThreshMS = 2000;
	const ui32 MinProgTimeMS = 500;

	// Windows Filetime is in 100 nanoseconds intervals
	ui32 MSToFiletime = 10000;
	ui32 Sleep100NS = SleepMS * MSToFiletime;

	FILETIME SysTimeFile;
	GetSystemTimeAsFileTime(&SysTimeFile);
	ULARGE_INTEGER SysTime = ULARGE_INTEGER{SysTimeFile.dwLowDateTime, SysTimeFile.dwHighDateTime};

	// Calculates the time remainder the timer should adhere to. See Sleep() below for further explanations.
	ui64 TimePrecisionRem = SysTime.QuadPart % (Sleep100NS);
	ui64 Iterations = 0;
	ui64 TimeErrorCount = 0, IndivTimeErrorCount = 0;

	LARGE_INTEGER QueryFreq;
	QueryPerformanceFrequency(&QueryFreq);
	LARGE_INTEGER LastProcBegin = {}, ProcBegin = {}, ProcEnd = {};
	bool WasIdle = false;
	char CurTitle[511] = "\0", CurProcBuf[MAX_PATH] = "\0";
	bool ActivityValid = false;

	DWORD PrevLastInput = 0;
	ui64 IdleTime = 0;
	i32 ProcSymValue;

	// TODO NOW: Set the hour to the last logged file hour
	ui64 HourLast = 0;
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

	LARGE_INTEGER WholeProgramBegin = {}, WholeProgramEnd = {};
	QueryPerformanceCounter(&WholeProgramBegin);

	while(Running)
	{
		bool TimeChange = false;
		SYSTEMTIME SysTimeNow;
		FILETIME FileTimeNow;
		GetSystemTime(&SysTimeNow);
		SystemTimeToFileTime(&SysTimeNow, &FileTimeNow);
		ui64 CurHour = ULARGE_INTEGER{FileTimeNow.dwLowDateTime, FileTimeNow.dwHighDateTime}.QuadPart / (3LL * 1000LL * 1000LL * 10LL);
		// TODO NOW: Remove
		if(!HourLast)
		{
			HourLast = CurHour;
		}

		if(HourLast != CurHour)
		{
			Log(TE_Other, 0x0e, "Hour changed: Finishing and logging current activity.\n");

			TimeChange = true;
			HourLast = CurHour;
		}
		else
		{
			/*
				We do not want to check for idling if we are already switching hour, as that would cause 
				two simulatenous activity changes if we became idle now (there can only be one change at a time). 
			*/

			// TODO: Add code to detect whether a video is playing, and don't count watching videos as idle time.
			// By not directly using LastInputInfo's time, only comparing to it, we prevent overflow problems for integers.
			LASTINPUTINFO LastInputInfo = {sizeof(LASTINPUTINFO), 0};
			assert(GetLastInputInfo(&LastInputInfo));
			if(PrevLastInput != LastInputInfo.dwTime)
			{
				PrevLastInput = LastInputInfo.dwTime;
				IdleTime = SleepMS;
			}
			else
			{
				IdleTime += SleepMS;
			}
		}

		bool IsIdle = (IdleTime >= InactivityThreshMS);
		bool SwitchToIdle = (!WasIdle && IsIdle);
		bool SwitchFromIdle = (WasIdle && !IsIdle);

		// If the user isn't idle, check if the program changed
		bool ProgramChange = false;
		char NewTitle[511];
		ui32 NewTitleLen = 0;
		HWND ActiveWin = {};
		if(TimeChange)
		{
			strcpy_s(NewTitle, "Hour Switch");
		}
		else if(!IsIdle)
		{
			if(ActiveWin = GetForegroundWindow())
			{
				wchar_t NewTitleW[255];
				NewTitleLen = GetWindowText(ActiveWin, NewTitleW, ArrayLength(NewTitleW));

				if(NewTitleLen)
				{
					NarrowUTF(NewTitle, ArrayLength(NewTitle), NewTitleW);
				}
				else
				{
					strcpy_s(NewTitle, "Unknown");
				}

				if(strcmp(CurTitle, NewTitle) != 0)
				{
					ProgramChange = true;
				}
			}
		}
		else if(SwitchToIdle)
		{
			strcpy_s(NewTitle, "Idle");
		}

		bool ActivityChange = ProgramChange || SwitchToIdle || SwitchFromIdle;
		bool ProcedureChange = ActivityChange || TimeChange;
		assert(ProcedureChange == (TimeChange != ActivityChange));

		// If the active process is changing or the user is becoming idle, log the last active process.
		// NOTE that these conditions are exclusive, and ARE ASSUMED AS SUCH in the following code.
		if(ProcedureChange)
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
			if(SwitchToIdle)
			{
				ui64 AddedIdleTime = IdleTime * QueryFreq.QuadPart / 1000;
				ProcEnd.QuadPart -= AddedIdleTime;
				NextProcBegin.QuadPart -= AddedIdleTime;
			}

			// TODO: Use integer time
			// Note: The program time can be negative if the user became idle before the program started, so we need to check for that...
			f64 fProcTime = ((f64)(ProcEnd.QuadPart - ProcBegin.QuadPart) / (f64)QueryFreq.QuadPart);
			if(ActivityValid && fProcTime > 0)
			{
				if(SwitchFromIdle)
				{
					ProcSymValue = -1;
				}
				else
				{
					// Convert process name to a symbol
					char *ProcName = strrchr(CurProcBuf, '\\') + 1;
					ProcSymValue = ProcessProcSym(&Symbols, ProcName);
					i32 FileValue = FindSymbolInFile(&Symbols, ProcName);
					assert(FileValue == ProcSymValue);
				}

				// TODO: Don't log title if it proves to be unnecessary (especially since titles use up a lot of memory...).
				// TODO: Add extra info to certain windows (e.g. tab/site info for browsers).
				// Log the result
				LogProgram(LogFile, CurTitle, ProcSymValue, "-", fProcTime);

#ifdef DEBUG
				// Assert that there are no time leaks (no 'lost' seconds) and that conservation of time is correct
				AccumTimeMS += fProcTime;
				if(!SwitchToIdle)
				{
					QueryPerformanceCounter(&WholeProgramEnd);
					f64 TotalProgramTime = ((f64)(WholeProgramEnd.QuadPart - WholeProgramBegin.QuadPart) / (f64)QueryFreq.QuadPart);
					f64 TimeError = fabs(TotalProgramTime - AccumTimeMS);
					if(TimeError >= 0.2f)
					{
						printf("TIME ERROR: %f (%f not %f)\n", TimeError, AccumTimeMS, TotalProgramTime);
						assert(false);
					}
					else
					{
						printf("Time Precision: %f (%f not %f)\n", TimeError, AccumTimeMS, TotalProgramTime);
					}
				}
#endif
			}
			else if(*CurTitle && (fProcTime <= 0 || !ActivityValid))
			{
				NextProcBegin.QuadPart -= (ui64)(fProcTime * QueryFreq.QuadPart);
				Log(TE_Reaction, 0x04, "Adding invalid program time ('%s' for %fs) to next program ('%s').\n", CurTitle, fProcTime, NewTitle);
			}

			if(TimeChange)
			{
				ActivityValid = false;
			}
			else if(IsIdle)
			{
				ActivityValid = true;
			}
			else
			{
				// A program whose title or process name we can't decipher is not valid.
				ActivityValid = NewTitleLen && GetActiveProgramInfo(ActiveWin, CurProcBuf, ArrayLength(CurProcBuf));
			}

			LastProcBegin = ProcBegin;
			ProcBegin = NextProcBegin;

			Log(TE_Action, "Switching from '%s' to '%s'\n", CurTitle, NewTitle);
			strcpy_s(CurTitle, NewTitle);
		}

		WasIdle = IsIdle;

		if(Running)
		{
			// TODO: Once file checkpoints have been implemented, explain why perfect long-time precision is important.
			/*
				Sleep() doesn't guarantee perfect time precision, and these imprecisions can stack up to very
				high numbers over time. Here we try to manually correct these imprecisions.
				Example: If the sleep interval is one second, we try to keep the intervals at exactly one second.
					We do that by trying to keep the remainder of the second right (i.e. here in milliseconds):
					If the milliseconds for the first iteration is .517s, we try to continue that pattern:
					1.517s, 2.517s, etc. If the remainder is .520s, we sleep 3ms less.
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

	f64 TotalProgramTime = ((f64)(WholeProgramEnd.QuadPart - WholeProgramBegin.QuadPart) / (f64)QueryFreq.QuadPart);
	f64 TimeError = TotalProgramTime - AccumTimeMS;
	assert(TimeError < 0.2f); // TODO: Fix time-precision problems (including ASSURE_TIME_PRECISION())
	assert(!TimeErrorCount);

	assert(WaitForSingleObject(ThreadHnd, 1000) != WAIT_TIMEOUT);

	CloseHandle(LogFile);
	CloseHandle(Symbols.SymFile);

#ifndef DEBUG
	assert(SetFileAttributes(LogFilename, FILE_ATTRIBUTE_READONLY));
	assert(SetFileAttributes(SymFilename, FILE_ATTRIBUTE_READONLY));
#endif

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