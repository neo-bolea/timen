#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>

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

#define internal static;
#define global static;
#define persist static;

#ifdef DEBUG
#define assert(Expr) \
	if(!(Expr))        \
	{                  \
		*(i32 *)0 = 0;   \
	}                  \
	0
#else
#define assert(Expr) \
	Expr;              \
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

#define MAX(A, B) ((A) < (B)) ? (B) : (A)
#define MIN(A, B) ((A) < (B)) ? (A) : (B)


typedef enum eol_type
{
	LF,
	CRLF,
	CR,
	EOL_UNDEFINED
} eol_type;

global eol_type EOLType = EOL_UNDEFINED;
global char *EOLStr = "";
global ui32 EOLLen = 0;

void EOLCheck()
{
	assert(EOLType != EOL_UNDEFINED);
}

void UpdateEOLType(const char *Str)
{
	const char *C = strchr(Str, '\n');
	if(C)
	{
		if(C > Str && (*(C - 1) == '\r'))
		{
			EOLType = CRLF;
		}
		else
		{
			EOLType = LF;
		}
	}
	else
	{
		C = strchr(Str, '\r');
		if(C)
		{
			EOLType = CR;
		}
		else
		{
			EOLType = LF;
		}
	}

	switch(EOLType)
	{
		case LF:
			EOLStr = "\n";
			break;
		case CRLF:
			EOLStr = "\r\n";
			break;
		case CR:
			EOLStr = "\r";
			break;
		default:
			EOLStr = "";
			break;
	}
	EOLLen = strlen(EOLStr);
}

bool IsEOLChar(char C)
{
	EOLCheck();
	return ((EOLType == LF || EOLType == CRLF) && C == '\n') ||
				 ((EOLType == CR || EOLType == CRLF) && C == '\r');
}

// Compares strings until EOL or end of string
// EOLStr is the string that contains the new line character, Str is assumed to have no new line
bool StrCmpEOL(const char *MulLineStr, const char *Str)
{
	EOLCheck();
	while(*MulLineStr == *Str)
	{
		EOLStr++, Str++;
		if(!*MulLineStr || IsEOLChar(*MulLineStr))
		{
			return true;
		}
	}
	return false;
}

ui32 EOLCount(const char *Str)
{
	EOLCheck();
	ui32 Count = 0;
	while(*Str)
	{
		if(IsEOLChar(*Str++))
		{
			Count++;
			if(EOLType == CRLF)
			{
				Str++;
			}
		}
	}
	return Count;
}

char *AfterNextEOL(char *Str)
{
	EOLCheck();
	do
	{
		switch(EOLType)
		{
			case LF:
			case CRLF:
				if(*Str == '\n')
				{
					return Str + 1;
				}
				break;

			case CR:
				if(*Str == '\r')
				{
					return Str + 1;
				}
				break;
		}
	} while(*Str++);
	return 0;
}

char *InsertEOL(char *Str)
{
	switch(EOLType)
	{
		case LF:
			*Str++ = '\n';
			break;
		case CRLF:
			*Str++ = '\r';
			*Str++ = '\n';
			break;
		case CR:
			*Str++ = '\r';
			break;
	}
	return Str;
}

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

global const ui32 NEW_SYM_ID = (ui32)-1;
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
	assert(Syms->SymFile != INVALID_HANDLE_VALUE);
	assert(SetFilePointer(Syms->SymFile, 0, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER);
	LARGE_INTEGER FileSize64;
	DWORD BytesRead;
	assert(GetFileSizeEx(Syms->SymFile, &FileSize64));
	assert(FileSize64.QuadPart < UINT32_MAX);
	ui32 FileSize = (ui32)FileSize64.QuadPart;
	ui32 ID = 0;
	if(FileSize)
	{
		char *FileBuf = VirtualAlloc(0, FileSize + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		assert(FileBuf);
		assert(ReadFile(Syms->SymFile, FileBuf, FileSize, &BytesRead, 0) && BytesRead == FileSize);
		FileBuf[FileSize] = '\0';
		char *FilePos = FileBuf;
		while(true)
		{
			if(StrCmpEOL(FilePos, Str))
			{
				AddSymToTable(Syms, Str, Time, ID);
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

	// THe symbol doesn't exist yet, create it
	{
		process_symbol NewSym = AddSymToTable(Syms, Str, Time, NEW_SYM_ID);

		// Write the new symbol to the file
		DWORD BytesWritten;
		char *SymBufEnd = StrEnd(NewSym.Str);
		SymBufEnd = InsertEOL(SymBufEnd);
		*SymBufEnd = '\0';
		ui32 NewSymBufLen = SymBufEnd - NewSym.Str;
		assert(SetFilePointer(Syms->SymFile, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER);
		assert(WriteFile(Syms->SymFile, NewSym.Str, NewSymBufLen, &BytesWritten, 0));
		assert((i32)BytesWritten == NewSymBufLen);

		return NewSym.ID;
	}
}

int __stdcall WinMain(HINSTANCE hInstance,
											HINSTANCE hPrevInstance,
											LPSTR lpCmdLine,
											int nShowCmd)
{
	CreateDirectory("..\\data", 0);
	assert(SetCurrentDirectory("..\\data"));

	proc_sym_table Symbols;
	// By filling with zero we assure that unused symbol slots will be overwritten (since they will have a timestamp of 0, the oldest number)
	memset(&Symbols, 0, sizeof(proc_sym_table));

	const i32 SleepS = 1;
	const i32 InactivityThreshS = 2;
	char *LogFilename = "Log.txt";
	char *SymFilename = "Log.sym";
	HANDLE LogFile = CreateFile(LogFilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(LogFile != INVALID_HANDLE_VALUE);

	// TODO NOW: check that both files have same EOL type
	Symbols.SymFile = CreateFile(SymFilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(Symbols.SymFile != INVALID_HANDLE_VALUE);
	{
		LARGE_INTEGER FileSize64;
		DWORD BytesRead;
		assert(GetFileSizeEx(Symbols.SymFile, &FileSize64));
		ui32 FileSize = (ui32)FileSize64.QuadPart;
		char *FileBuf = VirtualAlloc(0, FileSize + 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		assert(ReadFile(Symbols.SymFile, FileBuf, FileSize, &BytesRead, 0) && BytesRead == FileSize);

		UpdateEOLType(FileBuf);
		// File should always end with a new line.
		// TODO NOW: Do the same for log file
		if(!IsEOLChar(FileBuf[FileSize - 1]))
		{
			DWORD BytesWritten;
			assert(WriteFile(Symbols.SymFile, EOLStr, EOLLen, &BytesWritten, 0) && BytesWritten == EOLLen);
		}
		Symbols.GUID = EOLCount(FileBuf);

		VirtualFree(FileBuf, FileSize, MEM_RELEASE);
	}

	DWORD PrevLastInput = 0;
	ui64 LastInputTime = 0;

	i32 Iterations = 100;
	while(Iterations--)
	{
		Sleep(SleepS * 1000);

		char ProgInfo[MAX_PATH * 2] = "\0";
		ui32 ProgInfoLen = 0;
		bool Failed = false;

		// By not directly using LastInputInfo's time, only comparing to it, we prevent overflow problems for i32.
		LASTINPUTINFO LastInputInfo = {sizeof(LASTINPUTINFO), 0};
		assert(GetLastInputInfo(&LastInputInfo));
		if(PrevLastInput != LastInputInfo.dwTime)
		{
			PrevLastInput = LastInputInfo.dwTime;
			LastInputTime = SleepS;
		}
		else
		{
			LastInputTime += SleepS;
		}

		if(LastInputTime >= InactivityThreshS)
		{
			strcpy_s(ProgInfo, ArrayLength(ProgInfo), "Idle\n");
			ProgInfoLen = strlen(ProgInfo);
		}
		else
		{
			HWND ActiveWin = GetForegroundWindow();
			if(ActiveWin)
			{
				char ProcNameBuf[MAX_PATH] = "\0";
				char *ProcName = 0;
				char Title[MAX_PATH] = "\0";
				ui32 TitleLen = 0, ProcNameLen = 0;
				DWORD ProcID;

				GetWindowThreadProcessId(ActiveWin, &ProcID);
				if(ProcID)
				{
					HANDLE FGProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, ProcID);
					if(FGProc)
					{
						ProcNameLen = GetProcessImageFileName(FGProc, ProcNameBuf, ArrayLength(ProcNameBuf));

						if(!ProcNameLen)
						{
							*ProcNameBuf = 0;
						}
						else
						{
							ProcName = strrchr(ProcNameBuf, '\\') + 1;
						}
					}
				}


				if(*ProcNameBuf)
				{
					TitleLen = GetWindowText(ActiveWin, Title, ArrayLength(Title));

					if(*Title)
					{
						i32 SymValue = ProcessProcSym(&Symbols, ProcName);
						wsprintf(ProgInfo, "%s%s%i (%s)%s", Title, EOLStr, SymValue, ProcName, EOLStr);
						ProgInfoLen = strlen(ProgInfo);
					}
					else
					{
						Failed = true;
					}
				}
				else
				{
					Failed = true;
				}
			}
		}

		// TODO: Only write to file if process name or title changed, otherwise increase a counter.
		if(!Failed)
		{
			assert(ProgInfoLen < ArrayLength(ProgInfo) + 1);
			char *ProgInfoEnd = ProgInfo + ProgInfoLen;
			ProgInfoEnd = InsertEOL(ProgInfoEnd);
			*ProgInfoEnd = '\0';
			ProgInfoLen = ProgInfoEnd - ProgInfo;

			DWORD BytesWritten;
			assert(WriteFile(LogFile, ProgInfo, ProgInfoLen, &BytesWritten, 0));
			assert(BytesWritten == ProgInfoLen);
			OutputDebugString(ProgInfo);
		}
	}

	assert(CloseHandle(LogFile));

	return 0;
}