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
_CreateFile(int Mode, const pchar *Path, int Access, int Share)
{
	ui32 WinAccess = WinProcessFileAccess(Access);
	ui32 WinShare = WinProcessFileShare(Share);
	wchar_t *Path16 = WidenUTFAlloc(Path);
	HANDLE File = CreateFile(Path16, WinAccess, WinShare, 0, Mode, FILE_ATTRIBUTE_NORMAL, 0);
	FreeUTF(Path16);
	return File;
}

internal void *
CreateNewFile(const pchar *Path, int Access, int Share = FS_Read, bool Overwrite = false)
{
	int OpenType = Overwrite ? CREATE_ALWAYS : CREATE_NEW;
	HANDLE File = _CreateFile(OpenType, Path, Access, Share);
	if(File == INVALID_HANDLE_VALUE)
	{
		pchar ErrorBuf[255];
		psprintf(ErrorBuf, PCHAR("ERROR (%i): Could not create a file (%s) that was set to %soverwrite.\n"),
						 GetLastError(), Path, Overwrite ? PCHAR("") : PCHAR("not "));
		DebugLogP(ErrorBuf);
		UNHANDLED_CODE_PATH;
		return 0;
	}
	return File;
}

internal void *
OpenFile(const pchar *Path, int Access, int Share = FS_Read)
{
	HANDLE File = _CreateFile(OPEN_EXISTING, Path, Access, Share);
	if(File == INVALID_HANDLE_VALUE)
	{
		pchar ErrorBuf[255];
		psprintf(ErrorBuf, PCHAR("ERROR (%i): Could not open a file (%s).\n"), GetLastError(), Path);
		DebugLogP(ErrorBuf);
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
		ichar ErrorBuf[255];
		psprintf(ErrorBuf, ("ERROR (%i): Could not close a file.\n"), GetLastError());
		DebugLogI(ErrorBuf);
		return false;
	}
	return true;
}

internal i64
GetFileSize(void *File)
{
	LARGE_INTEGER FileSizeL;
	if(!GetFileSizeEx(File, &FileSizeL) && FileSizeL.QuadPart < INT32_MAX)
	{
		ichar ErrorBuf[255];
		psprintf(ErrorBuf, ("ERROR (%i): Could not read the size of a file or it was too big.\n"), GetLastError());
		DebugLogI(ErrorBuf);
		UNHANDLED_CODE_PATH;
		return 0;
	}
	return FileSizeL.QuadPart;
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

// TODO: Change FileBytesWritten to (u)i64?
internal bool
WriteFileContents(void *FileHandle, void *Buffer, i32 BytesToWrite)
{
	DWORD FileBytesWritten;
	file_result Result;
	Result = (file_result)WriteFile(FileHandle, Buffer, BytesToWrite, &FileBytesWritten, 0);
	if((i32)FileBytesWritten != BytesToWrite || Result != file_result::Success)
	{
		ichar ErrorBuf[255];
		isprintf(ErrorBuf, ICHAR("ERROR (%i): Could not write to a file.\n"), GetLastError());
		DebugLogI(ErrorBuf);
		return false;
	}
	return true;
}

// NOTE: Return value must be freed with FreeFileContents.
internal void *
LoadFileContents(const pchar *Path, ui64 *FileSizeOut, i32 Access = FA_Read, i32 Share = FS_All)
{
	pchar ErrorBuf[255];
	void *File = OpenFile(Path, Access, Share);
	if(!File)
	{
		UNHANDLED_CODE_PATH;
		return 0;
	}

	i64 FileSize = GetFileSize(File);
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
			psprintf(ErrorBuf, PCHAR("ERROR (%i): Could not read all file (%s) contents, the result got truncated.\n"), GetLastError(), Path);
		}
		else if(ReadResult == file_result::Failure)
		{
			psprintf(ErrorBuf, PCHAR("ERROR (%i): Could not read file (%s) contents.\n"), GetLastError(), Path);
		}
		else
		{
			UNDEFINED_CODE_PATH;
		}
		DebugLogP(ErrorBuf);
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
	assert(FileContents);
	VirtualFree(FileContents, 0, MEM_RELEASE);
}