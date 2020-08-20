// TODO: Remove these variables once time-precision debugging is finished.
global t_file WholeProgramBegin = {}, WholeProgramEnd = {};
global t_min AccumTimeS = 0;
global uint TimeErrorCount = 0;

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

typedef struct activity_info
{
	activity_type Type = AT_Invalid;
	char Title[511] = "\0";
	char ProcessName[MAX_PATH] = "\0";

	activity_info &operator=(activity_info &B);
} activity_info;

internal char *
GetActivityTitle(activity_info &Act)
{
	switch(Act.Type)
	{
		case AT_Prog:
			return Act.Title;
		case AT_Unknown:
			return "Unknown";
		case AT_Idle:
			return "Idle";
		default:
			return "???";
	}
}

bool operator==(activity_info &A, activity_info &B)
{
	if(A.Type != B.Type)
	{
		return false;
	}

	if(A.Type == AT_Prog)
	{
		return strcmp(A.Title, B.Title) == 0;
	}
	else
	{
		return true;
	}
}

bool operator!=(activity_info &A, activity_info &B)
{
	return !(A == B);
}

activity_info &activity_info::operator=(activity_info &B)
{
	activity_info &A = *this;
	if(A != B)
	{
		char *LogTitleA = GetActivityTitle(A), *LogTitleB = GetActivityTitle(B);
		Assert(strcmp(LogTitleA, LogTitleB) != 0);
		Log("Switching from '%s' to '%s'\n", LogTitleA, LogTitleB);
		strcpy_s(A.Title, B.Title);
		strcpy_s(A.ProcessName, B.ProcessName);
		A.Type = B.Type;
	}
	return A;
}

char *ParseActivity(char *Str, i32 *ActSym, t_min *Time)
{
#define NEXT_EOL()               \
	if(!(Str = AfterNextEOL(Str))) \
	{                              \
		return 0;                    \
	}                              \
	0

	NEXT_EOL();
	*ActSym = atoi(Str);
	NEXT_EOL();
	NEXT_EOL();
	*Time = (t_min)ParseLogTime(Str);
	// Assert(ATime > MinProgramTime);
	NEXT_EOL();
	NEXT_EOL();

#undef NEXT_EOL
	return Str;
}

internal void
LogActivity(HANDLE File, process_symbol_table &Symbols,
						char *Title, char *NextTitle, char *ProcessName, const char *Info,
						activity_type ActivityType, activity_type NextActivityType,
						t_file TimeBegin, t_file TimeEnd, t_file &NextTimeBegin, t_file QueryFreq)
{
	Assert(strcmp(Title, NextTitle) != 0);

	// Note: The program time can be negative if the user became idle before the program started, so we need to check for that...
	t_min ActivityTime = ((t_min)(TimeEnd - TimeBegin) / (t_min)QueryFreq);
	if(ActivityIsLoggable(ActivityType) && ActivityTime > 0)
	{
		i32 ProcSymValue = -1;
		if(ActivityType != AT_Idle)
		{
			// Convert process name to a symbol
			// TODO: Handle different types of slashes.
			char *ProcName = strrchr(ProcessName, '\\') + 1;
			ProcSymValue = ProcessProcSym(&Symbols, ProcName);
			Assert((i32)FindSymbolInFile(&Symbols, ProcName) == ProcSymValue);
		}

		// TODO: Don't log title if it proves to be unnecessary (especially since titles use up a lot of memory...).
		// TODO: Add extra info to certain windows (e.g. tab/site info for browsers).
		{
			ui64 TitleLen = strlen(Title);
			Assert(TitleLen);
			// TODO: Fix this (this was a temp fix).
			char LogBuffer[1024];
			if(TitleLen > 1000)
			{
				Title[1000] = '\0';
			}
			sprintf_s(LogBuffer, "%s%s%i%s%s%s%" P_TMIN "%s%s", Title, EOLStr, ProcSymValue, EOLStr, Info, EOLStr, ActivityTime, EOLStr, EOLStr);
			Log(0x02, "Logging '%s' for %" P_TMIN "s\n", Title, ActivityTime);
			i32 LogLen = (i32)strlen(LogBuffer);

			DWORD BytesWritten;
			Assert(SetFilePointer(File, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER);
			Assert(WriteFile(File, LogBuffer, LogLen, &BytesWritten, 0) && (i32)BytesWritten == LogLen);
		}

#ifdef DEBUG
		// Assert that there are no time leaks (no 'lost' seconds) and that conservation of time is correct.
		// Idle time is an exception, since we might later 'reclaim' time as idle, which would make the assertion temporarily a false positive.
		AccumTimeS += ActivityTime;
		if(NextActivityType != AT_Idle)
		{
			QueryPerformanceCounter((LARGE_INTEGER *)&WholeProgramEnd);
			t_min TotalProgramTime = ((t_min)(WholeProgramEnd - WholeProgramBegin) / (t_min)QueryFreq);
			t_min TimeError = abs(TotalProgramTime - AccumTimeS);
			if(TimeError >= 0.2f)
			{
				Log(0x4c, "TIME ERROR: %" P_TMIN " (%" P_TMIN " not %" P_TMIN ")\n", TimeError, AccumTimeS, TotalProgramTime);
				TimeErrorCount++;
			}
			else
			{
				Log("Time Precision: %" P_TMIN " (%" P_TMIN " not %" P_TMIN ")\n", TimeError, AccumTimeS, TotalProgramTime);
			}
		}
#endif
	}
	else if(ActivityTime <= 0 || !ActivityIsLoggable(ActivityType))
	{
		// If the current activity is somehow invalid, add the time to the next activity.
		NextTimeBegin -= (ui64)(ActivityTime * QueryFreq);
		Log(0x04, "Adding invalid program time ('%s' for %" P_TMIN "s) to next program ('%s').\n", Title, ActivityTime, NextTitle);
	}
}

internal bool
GetActiveProgramInfo(activity_info &Info)
{
	Info.Type = AT_Unknown;

	constexpr ui32 ProcBufLen = ArrLen(Info.ProcessName);
	constexpr ui32 TitleLen = ArrLen(Info.Title);

	HWND ActiveWin = GetForegroundWindow();
	if(!ActiveWin)
	{
		return false;
	}

	DWORD ProcID;
	if(!GetWindowThreadProcessId(ActiveWin, &ProcID))
	{
		return false;
	}

	Assert(ProcID);
	HANDLE FGProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, ProcID);
	if(FGProc == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	wchar_t ProcBufW[ProcBufLen];
	if(!GetProcessImageFileName(FGProc, ProcBufW, ArrLen(ProcBufW)))
	{
		return false;
	}

	wchar_t NewTitleW[ArrLen(Info.Title)];
	if(!GetWindowText(ActiveWin, NewTitleW, ArrLen(NewTitleW)))
	{
		// TODO: How to handle no window title? Ignore? Use exe name (like it is now)?
		// TODO: Handle different types of slashes.
		wcscpy_s(NewTitleW, wcsrchr(ProcBufW, '\\') + 1);
	}

	NarrowUTF(Info.ProcessName, ProcBufLen, ProcBufW);
	NarrowUTF(Info.Title, TitleLen, NewTitleW);
	Info.Type = AT_Prog;
	return true;
}

void GetNextActivity(t_min &IdleTime, DWORD &PrevLastInput, t_min SleepTime, t_min InactivityThresh, activity_info &NextA)
{
	// TODO: Add code to detect whether a video is playing, and don't count watching videos as idle time.
	// By not directly using LastInputInfo's time, only comparing to it, we prevent overflow problems for integers.
	LASTINPUTINFO LastInputInfo = {sizeof(LASTINPUTINFO), 0};
	Assert(GetLastInputInfo(&LastInputInfo));
	if(PrevLastInput != LastInputInfo.dwTime)
	{
		PrevLastInput = LastInputInfo.dwTime;
		IdleTime = 0;
	}
	IdleTime += SleepTime;

	if(IdleTime >= InactivityThresh)
	{
		NextA.Type = AT_Idle;
	}
	else
	{
		// If the user isn't idle, check if the program changed
		GetActiveProgramInfo(NextA);
	}
}