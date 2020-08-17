enum file_access
{
	FA_None = 0,

	FA_Read = 0b001,
	FA_Write = 0b010,
	FA_Execute = 0b100,
	FA_All = 0b111,
};
enum file_share
{
	FS_None = 0,

	FS_Read = 0b001,
	FS_Write = 0b010,
	FS_Delete = 0b100,
	FS_All = 0b111,
};
enum class file_result
{
	Failure = 0,
	Success = 1,
	EndOfFile
};

internal int
WinProcessFileAccess(int Access)
{
	int Result = 0;
	if(Access & FA_Read)
	{
		Result |= GENERIC_READ;
	}
	if(Access & FA_Write)
	{
		Result |= GENERIC_WRITE;
	}
	if(Access & FA_Execute)
	{
		Result |= GENERIC_EXECUTE;
	}
	return Result;
}

internal int
WinProcessFileShare(int Share)
{
	int Result = 0;
	if(Share & FS_Read)
	{
		Result |= FILE_SHARE_READ;
	}
	if(Share & FS_Write)
	{
		Result |= FILE_SHARE_WRITE;
	}
	if(Share & FS_Delete)
	{
		Result |= FILE_SHARE_DELETE;
	}
	return Result;
}

internal void *
_CreateFile(int Mode, const char *Path, int Access, int Share)
{
	ui32 WinAccess = WinProcessFileAccess(Access);
	ui32 WinShare = WinProcessFileShare(Share);
	wchar_t Path16[MAX_PATH];
	WidenUTF(Path16, ArrLen(Path16), Path);
	HANDLE File = CreateFile(Path16, WinAccess, WinShare, 0, Mode, FILE_ATTRIBUTE_NORMAL, 0);
	return File;
}

internal void *
CreateNewFile(const char *Path, int Access, int Share = FS_Read, bool Overwrite = false)
{
	int OpenType = Overwrite ? CREATE_ALWAYS : CREATE_NEW;
	HANDLE File = _CreateFile(OpenType, Path, Access, Share);
	if(File == INVALID_HANDLE_VALUE)
	{
		char ErrorBuf[255];
		sprintf_s(ErrorBuf, PCHAR("ERROR (%i): Could not create a file (%s) that was set to %soverwrite.\n"),
						 GetLastError(), Path, Overwrite ? PCHAR("") : PCHAR("not "));
		Log(0x4c, ErrorBuf);
		UNHANDLED_CODE_PATH;
		return 0;
	}
	return File;
}

internal void *
OpenFile(const char *Path, int Access, int Share = FS_Read)
{
	HANDLE File = _CreateFile(OPEN_EXISTING, Path, Access, Share);
	if(File == INVALID_HANDLE_VALUE)
	{
		char ErrorBuf[255];
		sprintf_s(ErrorBuf, PCHAR("ERROR (%i): Could not open a file (%s).\n"), GetLastError(), Path);
		Log(0x4c, ErrorBuf);
		UNHANDLED_CODE_PATH;
		return 0;
	}

	return File;
}

internal bool
CloseFile(void *File)
{
	if(!CloseHandle(File))
	{
		char ErrorBuf[255];
		sprintf_s(ErrorBuf, ("ERROR (%i): Could not close a file.\n"), GetLastError());
		Log(0x4c, ErrorBuf);
		return false;
	}
	return true;
}

internal ui32
GetFileSize32(HANDLE File)
{
	LARGE_INTEGER FileSize64;
	Assert(GetFileSizeEx(File, &FileSize64));
	Assert(FileSize64.QuadPart < UINT32_MAX);
	return (ui32)FileSize64.QuadPart;
}

// TODO: Change FileBytesRead to (u)i64?
internal file_result
ReadFileContents(void *FileHandle, void *Buffer, i32 BytesToRead)
{
	DWORD FileBytesRead;
	file_result Result;
	Result = (file_result)ReadFile(FileHandle, Buffer, BytesToRead, &FileBytesRead, 0);
	if(Result == file_result::Success && (i32)FileBytesRead != BytesToRead)
	{
		Result = file_result::EndOfFile;
	}
	return Result;
}

internal void *
ReadFileContents(void *FileHandle, i32 BytesToRead, file_result *Result)
{
	void *Buffer = VirtualAlloc(0, BytesToRead, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	*Result = ReadFileContents(FileHandle, Buffer, BytesToRead);
	return Buffer;
}

// TODO: Change FileBytesWritten to (u)i64?
internal bool
WriteFileContents(void *FileHandle, void *Buffer, i32 BytesToWrite)
{
	DWORD FileBytesWritten;
	file_result Result;
	Result = (file_result)WriteFile(FileHandle, Buffer, BytesToWrite, &FileBytesWritten, 0);
	if((i32)FileBytesWritten != BytesToWrite || Result != file_result::Success)
	{
		char ErrorBuf[255];
		sprintf_s(ErrorBuf, "ERROR (%i): Could not write to a file.\n", GetLastError());
		Log(0x4c, ErrorBuf);
		return false;
	}
	return true;
}

// NOTE: Return value must be freed with FreeFileContents.
internal void *
LoadFileContents(const char *Path, ui64 *FileSizeOut, i32 Access = FA_Read, i32 Share = FS_All)
{
	char ErrorBuf[255];
	void *File = OpenFile(Path, Access, Share);
	if(!File)
	{
		UNHANDLED_CODE_PATH;
		return 0;
	}

	i64 FileSize = GetFileSize32(File);
	if(!FileSize)
	{
		UNHANDLED_CODE_PATH;
		return 0;
	}

	void *FileBuffer = VirtualAlloc(0, FileSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	file_result ReadResult = ReadFileContents(File, FileBuffer, (i32)FileSize);
	if(ReadResult != file_result::Success)
	{
		if(ReadResult == file_result::EndOfFile)
		{
			sprintf_s(ErrorBuf, PCHAR("ERROR (%i): Could not read all file (%s) contents, the result got truncated.\n"), GetLastError(), Path);
		}
		else if(ReadResult == file_result::Failure)
		{
			sprintf_s(ErrorBuf, PCHAR("ERROR (%i): Could not read file (%s) contents.\n"), GetLastError(), Path);
		}
		else
		{
			UNDEFINED_CODE_PATH;
		}
		Log(0x4c, ErrorBuf);
		CloseFile(File);
		UNHANDLED_CODE_PATH;
		return 0;
	}

	if(FileSizeOut)
	{
		*FileSizeOut = FileSize;
	}

	CloseFile(File);
	return FileBuffer;
}

internal void
FreeFileContents(void *FileContents)
{
	Assert(FileContents);
	VirtualFree(FileContents, 0, MEM_RELEASE);
}