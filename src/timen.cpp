#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <rpc.h>
#include <shellapi.h>
#include <Shlwapi.h>

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

std::atomic_bool Running = true;
DWORD WINAPI NotifIconThread(void *Args);

// TODO: Unicode (UTF-8) support
int __stdcall WinMain(HINSTANCE hInstance,
											HINSTANCE hPrevInstance,
											LPSTR lpCmdLine,
											int nShowCmd)
{
	DWORD ThreadID;
	HANDLE ThreadHnd = CreateThread(0, 0, NotifIconThread, hInstance, 0, &ThreadID);
	assert(ThreadHnd != INVALID_HANDLE_VALUE);

	const char *DataDir = "..\\data";
	CreateDirectory(DataDir, 0);
	assert(SetCurrentDirectory(DataDir));

	char *LogFilename = "Log.txt";
	char *SymFilename = "Log.sym";

	proc_sym_table Symbols;
	// By filling with zero we assure that unused symbol slots will be overwritten (since they will have a timestamp of 0, the oldest number)
	memset(&Symbols, 0, sizeof(proc_sym_table));

	const ui32 SleepS = 1;
	const i32 InactivityThreshS = 2;
	if(PathFileExists(LogFilename))
	{
		assert(SetFileAttributes(LogFilename, FILE_ATTRIBUTE_NORMAL));
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

	DWORD PrevLastInput = 0;
	ui64 LastInputTime = 0;
	i32 ProcSymValue;
	while(Running)
	{
		// By not directly using LastInputInfo's time, only comparing to it, we prevent overflow problems for integers.
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


		printf("Next...\n");
		Sleep(1000 * SleepS);

		bool WriteProc = false;

		if(LastInputTime >= InactivityThreshS)
		{
			ProcSymValue = -1;
			WriteProc = true;
		}
		else
		{
			HWND ActiveWin = GetForegroundWindow();
			char Title[MAX_PATH] = "\0";
			if(ActiveWin && GetWindowText(ActiveWin, Title, ArrayLength(Title)))
			{
				DWORD ProcID;
				if(GetWindowThreadProcessId(ActiveWin, &ProcID))
				{
					assert(ProcID);
					HANDLE FGProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, ProcID);
					if(FGProc)
					{
						char ProcNameBuf[MAX_PATH] = "\0";
						if(GetProcessImageFileName(FGProc, ProcNameBuf, ArrayLength(ProcNameBuf)))
						{
							char *ProcName = strrchr(ProcNameBuf, '\\') + 1;
							ProcSymValue = ProcessProcSym(&Symbols, ProcName);
							//wsprintf(ProgInfo, "%s%s%i for %is (%s)%s", Title, EOLStr, SymValue, ProcDuration, ProcName, EOLStr);

							ui32 FileValue = FindSymbolInFile(&Symbols, ProcName);
							assert(FileValue == ProcSymValue);

							WriteProc = true;
						}
					}
				}
			}
		}

		// TODO: Only write to file if process name or title changed, otherwise increase a counter.
		if(WriteProc)
		{
			assert(ProgInfoLen < ArrayLength(ProgInfo) + 1);
			char *ProgInfoEnd = ProgInfo + ProgInfoLen;
			ProgInfoEnd = InsertEOL(ProgInfoEnd);
			*ProgInfoEnd = '\0';
			ProgInfoLen = (ui32)(ProgInfoEnd - ProgInfo);

			DWORD BytesWritten;
			assert(SetFilePointer(LogFile, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER);
			assert(WriteFile(LogFile, ProgInfo, ProgInfoLen, &BytesWritten, 0));
			assert(BytesWritten == ProgInfoLen);
			OutputDebugString(ProgInfo);
		}
		else
		{
			int x;
		}
	}

	assert(WaitForSingleObject(ThreadHnd, 1000) != WAIT_TIMEOUT);

	CloseHandle(LogFile);
	CloseHandle(Symbols.SymFile);

#ifndef DEBUG
	assert(SetFileAttributes(LogFilename, FILE_ATTRIBUTE_READONLY));
	assert(SetFileAttributes(SymFilename, FILE_ATTRIBUTE_READONLY));
#endif

	// TODO: Save text files.

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
					InsertMenu(hPopMenu, (ui32)-1, MF_BYPOSITION | MF_STRING, IDM_EXIT, "Exit");
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

		case WM_QUIT:
		case WM_CLOSE:
			assert(false);
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
		WndClass.lpszClassName = "timen";
		WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
		assert(RegisterClassEx(&WndClass));
		Window = CreateWindow(WndClass.lpszClassName, "timen", WS_MINIMIZE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
		assert(Window != INVALID_HANDLE_VALUE);
		SetWindowLongPtr(Window, GWLP_USERDATA, (LONG_PTR)&WD);

		WD.NotifIcon = &NotifIcon;

		UUID IconUuid;
		assert(UuidCreateSequential(&IconUuid) == RPC_S_OK);

		memset(&NotifIcon, 0, sizeof(NOTIFYICONDATA));
		NotifIcon.cbSize = sizeof(NOTIFYICONDATA);
		NotifIcon.hWnd = Window;
		NotifIcon.uID = IconUuid.Data1;
		NotifIcon.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		NotifIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		NotifIcon.uCallbackMessage = WM_USER_SHELLICON;
		strcpy_s(NotifIcon.szTip, ArrayLength(NotifIcon.szTip), "timen");
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