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

typedef uint8_t  ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

#pragma function(memset)
void* memset(void* Dst, i32 V, size_t Count)
{
	i8* Bytes = (i8*)Dst;
	while (Count--)
	{
		*Bytes++ = (i8)V;
	}
	return Dst;
}

#ifdef DEBUG
#define assert(Expr) if(!(Expr)) { *(i32 *)0 = 0; }0
#else
#define assert(Expr) Expr; 0
#endif
#define ArrayLength(Array) (sizeof(Array) / sizeof(Array[0]))
#define PrintError(){ char Error[255]; wsprintf(Error, "ERROR: %i\n", GetLastError()); OutputDebugString(Error); }0

i32 StrLen(const char* Str)
{
	const char* c = Str;
	i32 Size = 0;
	while (*c++)
	{
		Size++;
	}
	return Size;
}

void StrCpy(char* Dst, const char* Src)
{
	const char* c = Src;
	do
	{
		*Dst++ = *c;
	} while (*c++);
}

i32 StrCmpC(const char* A, const char* B, char EndChar)
{
	while (*A == EndChar && (*A == *B))
	{
		A++, B++;
	}
	return *(const unsigned char*)A - *(const unsigned char*)B;
}

i32 StrCmp(const char* A, const char* B)
{
	return StrCmpC(A, B, '\0');
}

char* StrChr(char* Str, char Char)
{
	while (*Str != Char) { Str++; }
	return Str;
}

int AToI(char* Str)
{
	i32 Result = 0;
	for (i32 i = 0; Str[i]; i++)
	{
		Result = Result * 10 + Str[i] - '0';
	}
	return Result;
}

void main();
void WinMainCRTStartup()
{
	main();
}

typedef struct process_symbol
{
	char Str[MAX_PATH];
	ui64 LastActive;
	ui32 ID;
} process_symbol;

typedef struct proc_sym_table
{
	process_symbol Symbols[32];
	HANDLE SymFile;
	ui32 GUID;
	ui32 SymbolCount;
} proc_sym_table;

void SerializeProcSym(char* Buf, process_symbol* Sym)
{
	wsprintf(Buf, "%s\n%i\n", Sym->Str, Sym->ID);
}

ui32 ProcessProcSym(proc_sym_table* Syms, const char* Str)
{
	SYSTEMTIME SysTime;
	FILETIME FileTime;
	GetSystemTime(&SysTime);
	assert(SystemTimeToFileTime(&SysTime, &FileTime));
	ui64 Time = (ui64)FileTime.dwLowDateTime | ((ui64)FileTime.dwHighDateTime << 32);

	for (ui32 i = 0; i < Syms->SymbolCount; i++)
	{
		if (StrCmp(Syms->Symbols[i].Str, Str) == 0)
		{
			Syms->Symbols[i].LastActive = Time;
			return Syms->Symbols[i].ID;
		}
	}

	// TODO NOW: Find process_symbol in file
	assert(Syms->SymFile);
	assert(SetFilePointer(Syms->SymFile, 0, 0, FILE_BEGIN));
	LARGE_INTEGER FileSize;
	DWORD BytesRead;
	GetFileSizeEx(Syms->SymFile, &FileSize);
	assert(FileSize.QuadPart < UINT32_MAX);
	char* FileBuf = VirtualAlloc(0, FileSize.QuadPart, MEM_RESERVE | MEM_COMMIT, 0);
	assert(ReadFile(Syms->SymFile, FileBuf, (ui32)FileSize.QuadPart, &BytesRead, 0) && BytesRead == (ui32)FileSize.QuadPart);
	char* FilePos = FileBuf;
	while (true)
	{
		if (StrCmpC(Str, FilePos, '\n') == 0)
		{
			char IDBuf[32];
			StrCpy(IDBuf, FilePos);
			return AToI(IDBuf);
		}

		for (ui32 i = 0; i < 2; i++)
		{
			FilePos = StrChr(FilePos, '\n') + 1;
			if (!FilePos) { break; }
		}
	}
	VirtualFree(FileBuf, FileSize.QuadPart, MEM_RELEASE);

	// THe symbol doesn't exist yet, create it
	{
		// Find the oldest symbol, which we will overwrite
		ui64 OldestTime = UINT64_MAX;
		ui32 OldestIndex = 0;
		for (ui32 i = 0; i < Syms->SymbolCount; i++)
		{
			ui64 CurTime = Syms->Symbols[i].LastActive;
			if (CurTime < OldestTime)
			{
				OldestTime = CurTime;
				OldestIndex = i;
			}
		}

		// Overwrite the oldest symbol
		process_symbol NewSym;
		StrCpy(NewSym.Str, Str);
		NewSym.ID = Syms->GUID++;
		Syms->Symbols[OldestIndex] = NewSym;

		// Write the new symbol to the file
		char NewSymBuf[512];
		SerializeProcSym(NewSymBuf, &NewSym);
		DWORD BytesWritten;
		i32 NewSymBufLen = StrLen(NewSymBuf);
		SetFilePointer(Syms->SymFile, 0, 0, FILE_END);
		assert(WriteFile(Syms->SymFile, NewSymBuf, NewSymBufLen, &BytesWritten, 0));
		assert((i32)BytesWritten == NewSymBufLen);

		return NewSym.ID;
	}
}

void main()
{
	int i = 1;
	STARTUPINFO StartUpInfo;
	PROCESS_INFORMATION ProcInfo;
	char* Args = " 10 1000";
	if (!CreateProcess(".\\proc_spammer.exe",
		Args,
		0,
		0,
		false,
		CREATE_NEW_CONSOLE,
		0,
		0,
		&StartUpInfo,
		&ProcInfo))
	{
		int Err = GetLastError();
		Err = Err;
	}

	while (true) {}
	if (++i)
	{
		return;
	}

	CreateDirectory("..\\data", 0);
	assert(SetCurrentDirectory("..\\data"));

	proc_sym_table Symbols;
	memset(&Symbols, 0, sizeof(proc_sym_table));

	const i32 SleepS = 1;
	const i32 InactivityThreshS = 2;
	char* LogFilename = "Log.txt";
	char* SymFilename = "Log.sym";
	HANDLE LogFile = CreateFile(LogFilename, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(LogFile != INVALID_HANDLE_VALUE);

	Symbols.SymFile = CreateFile(SymFilename, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(Symbols.SymFile != INVALID_HANDLE_VALUE);

	DWORD PrevLastInput = 0;
	ui64 LastInputTime = 0;

	i32 Iterations = 10;
	while (Iterations--)
	{
		Sleep(SleepS * 1000);

		char ProgInfo[MAX_PATH * 2] = "\0";
		ui32 ProgInfoLen = 0;
		bool Failed = false;

		// By not directly using LastInputInfo's time, only comparing to it, we prevent overflow problems for i32.
		LASTINPUTINFO LastInputInfo = { sizeof(LASTINPUTINFO), 0 };
		assert(GetLastInputInfo(&LastInputInfo));
		if (PrevLastInput != LastInputInfo.dwTime)
		{
			PrevLastInput = LastInputInfo.dwTime;
			LastInputTime = SleepS;
		}
		else
		{
			LastInputTime += SleepS;
		}

		if (LastInputTime >= InactivityThreshS)
		{
			StrCpy(ProgInfo, "Idle\n");
			ProgInfoLen = StrLen(ProgInfo);
		}
		else
		{
			HWND ActiveWin = GetForegroundWindow();
			if (ActiveWin)
			{
				char ProcName[MAX_PATH] = "\0";
				char Title[MAX_PATH] = "\0";
				ui32 TitleLen = 0, ProcNameLen = 0;
				DWORD ProcID;

				GetWindowThreadProcessId(ActiveWin, &ProcID);
				if (ProcID)
				{
					HANDLE FGProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, ProcID);
					if (FGProc)
					{
						ProcNameLen = GetModuleBaseName(FGProc, 0, ProcName, ArrayLength(ProcName));
						if (!ProcNameLen)
						{
							*ProcName = 0;
							Failed = true;
						}
					}
				}

				TitleLen = GetWindowText(ActiveWin, Title, ArrayLength(Title));

				if (*Title && *ProcName)
				{
					i32 SymValue = ProcessProcSym(&Symbols, ProcName);
					wsprintf(ProgInfo, "%s\n%i (%s)\n", Title, SymValue, ProcName);
					//wsprintf(ProgInfo, "%s\n%s\n", Title, ProcName);
					ProgInfoLen = StrLen(ProgInfo);
				}
				else
				{
					Failed = true;
				}
			}
		}

		if (!Failed)
		{
			assert(ProgInfoLen < ArrayLength(ProgInfo) + 1);
			ProgInfo[ProgInfoLen++] = '\n';
			ProgInfo[ProgInfoLen] = '\0';

			DWORD BytesWritten;
			assert(WriteFile(LogFile, ProgInfo, ProgInfoLen, &BytesWritten, 0));
			assert(BytesWritten == ProgInfoLen);
			OutputDebugString(ProgInfo);
		}
	}

	assert(CloseHandle(LogFile));

	//{

	//
	//	i32 Index;
	//	proc_sym_table* Syms = &Symbols;
	//	IterateChunks(Syms, Index,
	//		{
	//			DWORD BytesWritten;
	//			i32 SymLen = StrLen(Syms->Symbols[Index]);
	//			Syms->Symbols[Index][SymLen++] = '\n';
	//			assert(WriteFile(SymbolFile, Syms->Symbols[Index], SymLen, &BytesWritten, 0));
	//			assert((i32)BytesWritten == SymLen);
	//		});
	//
	//	assert(CloseHandle(SymbolFile));
	//}
}