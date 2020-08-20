typedef struct process_symbol
{
	char Str[MAX_PATH];
	ui64 LastActive;
	ui32 ID;
} process_symbol;

typedef struct process_symbol_table
{
	process_symbol Symbols[4];
	HANDLE SymFile;
	ui32 GUID;
} process_symbol_table;

internal ui32
FindSymbolInFile(process_symbol_table *Syms, const char *Str)
{
	Assert(Syms->SymFile != INVALID_HANDLE_VALUE);
	ui32 FileSize = GetFileSize32(Syms->SymFile);
	if(FileSize)
	{
		char *FileBuf = (char *)VirtualAlloc(0, FileSize + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		Assert(FileBuf);

		Assert(SetFilePointer(Syms->SymFile, 0, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER);
		DWORD BytesRead;
		Assert(ReadFile(Syms->SymFile, FileBuf, FileSize, &BytesRead, 0) && BytesRead == FileSize);
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
AddSymToTable(process_symbol_table *Syms, const char *Str, ui64 Time, ui32 ID)
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
ProcessProcSym(process_symbol_table *Syms, const char *Str)
{
	Assert(*Str);

	SYSTEMTIME SysTime;
	FILETIME FileTime;
	GetSystemTime(&SysTime);
	Assert(SystemTimeToFileTime(&SysTime, &FileTime));
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
		Assert(SetFilePointer(Syms->SymFile, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER);
		Assert(WriteFile(Syms->SymFile, NewSym.Str, NewSymBufLen, &BytesWritten, 0));
		Assert((i32)BytesWritten == NewSymBufLen);

		return NewSym.ID;
	}
}

ui32 GetProcSymCount(timen_cfg &tcfg)
{
	ui64 FileSize;
	char *SymBuf = (char *)LoadFileContents(tcfg.SymFilename, &FileSize, FA_Read);
	char *SymPos = SymBuf;
	ui32 Count = 0;
	do
	{
		Count++;
	} while(SymPos = AfterNextEOL(SymPos));
	FreeFileContents(SymBuf);
	return Count;
}