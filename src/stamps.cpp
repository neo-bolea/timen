typedef ui64 byte_mark;
#define ParseByteMarker(Str) strtoull(Str, 0, 10)

typedef struct stamp_info
{
	// The marker which points to an activity in the main log file.
	byte_mark Marker;
	// The time at which that activity started.
	t_min Time;
} stamp_info;

void StampToTimeStr(const timen_cfg &tcfg, char *Buf, size_t BufLen, t_min Time)
{
	SYSTEMTIME SysTime;
	ULARGE_INTEGER Time64;
	Time64.QuadPart = Time * TMinToTFile + tcfg.GlobalTimeBegin;
	FILETIME FileTime, LocalFileTime;
	FileTime.dwLowDateTime = Time64.LowPart;
	FileTime.dwHighDateTime = Time64.HighPart;
	FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
	FileTimeToSystemTime(&LocalFileTime, &SysTime);
	ui32 TimeDivLen = sprintf_s(Buf, BufLen, "%i/%i/%i, %ih:%im:%is",
															SysTime.wMonth, SysTime.wDay, SysTime.wYear, SysTime.wHour, SysTime.wMinute, SysTime.wSecond);
}

void AddStamp(const timen_cfg &tcfg, HANDLE LogFile, HANDLE StampFile, t_min Time)
{
	char TimeDivBuf[255], StampBuf[255];
	LARGE_INTEGER LastActivityEnd;
	Assert(GetFileSizeEx(LogFile, &LastActivityEnd));
#ifdef DEBUG
	StampToTimeStr(tcfg, StampBuf, ArrLen(StampBuf), Time);
	ui32 TimeDivLen = sprintf_s(TimeDivBuf, "%llu %llu [%s]%s", Time, LastActivityEnd.QuadPart, StampBuf, EOLStr);
#else
	ui32 TimeDivLen = sprintf_s(TimeDivBuf, "%llu %llu%s", Time, LastActivityEnd.QuadPart, EOLStr);
#endif
	DWORD BytesWritten;
	Assert(SetFilePointer(StampFile, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER);
	Assert(WriteFile(StampFile, TimeDivBuf, TimeDivLen, &BytesWritten, 0) && BytesWritten == TimeDivLen);
}

t_min CreateTimestamp(timen_cfg &tcfg, uint Year, uint Month = 0, uint Day = 0, uint Hour = 0, uint Minute = 0, uint Second = 0)
{
	SYSTEMTIME SysTime = {};
	SysTime.wYear = (WORD)Year;
	SysTime.wMonth = (WORD)Month;
	SysTime.wDay = (WORD)Day;
	SysTime.wHour = (WORD)Hour;
	SysTime.wMinute = (WORD)Minute;
	SysTime.wSecond = (WORD)Second;

	TIME_ZONE_INFORMATION TimeZone;
	GetTimeZoneInformation(&TimeZone);

	FILETIME LocFileTime, FileTime;
	LocalSystemTimeToLocalFileTime(&TimeZone, &SysTime, &LocFileTime);
	LocalFileTimeToFileTime(&LocFileTime, &FileTime);

	t_min ResultTime = ULARGE_INTEGER{FileTime.dwLowDateTime, FileTime.dwHighDateTime}.QuadPart;
	Assert((t_file)ResultTime >= tcfg.GlobalTimeBegin);
	return (ResultTime - tcfg.GlobalTimeBegin) / TMinToTFile;
}

const uint StampRangeCapInc = 64;
typedef struct stamp_range
{
	stamp_info *Stamps;
	size_t Size, Capacity;

	stamp_info &operator[](size_t i)
	{
		Assert(i < Size);
		return Stamps[i];
	}
} stamp_range;

void FreeStampRange(stamp_range &Range)
{
	if(Range.Stamps)
	{
		free(Range.Stamps);
		Range.Size = 0;
		Range.Stamps = 0;
		Range.Capacity = 0;
	}
}

void AddStampToRange(stamp_range &Range, stamp_info &&Stamp)
{
	if(Range.Size >= Range.Capacity)
	{
		Assert(Range.Size == Range.Capacity);
		Range.Capacity += StampRangeCapInc;
		Range.Stamps = (stamp_info *)realloc(Range.Stamps, Range.Capacity * sizeof(stamp_info));
	}
	Range.Stamps[Range.Size++] = Stamp;
}

stamp_range GetTimeStampsInRange(char *StpBuf, ui64 StpFileSize, ui64 LogFileSize, t_min StartTime, t_min EndTime)
{
	if(StartTime >= EndTime)
	{
		UNHANDLED_CODE_PATH;
		return {};
	}

	bool FoundStart = false, FoundEnd = false, IsStart = false;
	stamp_range Range = {};

	char *FilePos = StpBuf;

	t_min LastStpDate = 0;
	byte_mark LastStpMarker = 0;
	while((ui64)(FilePos - StpBuf) < StpFileSize)
	{
		t_min StpDate = strtoull(FilePos, &FilePos, 10);
		char *StpMarkerStr = strchr(FilePos, ' ') + 1;
		byte_mark StpMarker = ParseByteMarker(StpMarkerStr);

		Assert(LastStpMarker <= StpMarker);
		Assert(LastStpDate <= StpDate);

		if(!FoundStart)
		{
			if(StpDate > StartTime)
			{
				Assert(!LastStpDate || LastStpDate <= StartTime);
				FoundStart = true;
				IsStart = true;
				FilePos++;

				if(LastStpDate)
				{
					AddStampToRange(Range, {LastStpMarker, LastStpDate});
				}
				else
				{
					AddStampToRange(Range, {StpMarker, StpDate});
				}
			}
		}

		if(FoundStart)
		{
			// Only add the stamp if it wasn't the first one in the file, as otherwise we have already added it.
			if(!IsStart || LastStpDate)
			{
				AddStampToRange(Range, {StpMarker, StpDate});
			}
			else
			{
				IsStart = false;
			}

			if(StpDate >= EndTime)
			{
				if(!AfterNextEOL(StpMarkerStr))
				{
					// TODO TRIVIAL: Necessary?
					// End of file, set the end of range at the end of file.
					AddStampToRange(Range, {LogFileSize, LastStpDate});
				}
				FoundEnd = true;
				break;
			}
		}

		FilePos = AfterNextEOL(StpMarkerStr);
		LastStpDate = StpDate;
		LastStpMarker = StpMarker;
	}

	// If the time window is outside the given range, there should not be any stamps added.
	if(!FoundStart)
	{
		Assert(Range.Size == 0);
	}
	else if(!FoundEnd)
	{
		// TODO REFACTOR: Clean up.
		if(Range[Range.Size - 1].Marker != LogFileSize)
		{
			AddStampToRange(Range, {LogFileSize, LastStpDate});
		}
	}

	return Range;
}