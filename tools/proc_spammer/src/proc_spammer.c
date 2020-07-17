#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <Shlwapi.h>

#include <stdbool.h>
#include <stdio.h>

#define ArrayLength(Array) (sizeof((Array)) / sizeof((Array)[0]))

#include <assert.h>

#define Assert(Expr) if((!Expr)) { printf("ERROR: %s\n", #Expr); getchar(); return; }0

#define MAX_EXE_NAME 32

int main(int argc, char** argv)
{
	int ProcCount = 0;
	int SleepMS = 0;
	if (argc == 3)
	{
		ProcCount = atoi(argv[1]);
		SleepMS = atoi(argv[2]);
	}

	if (!ProcCount || !SleepMS)
	{
#ifdef DEBUG
		ProcCount = 30;
		SleepMS = 1000;
#else
		printf("Usage: [ProcCount] [SleepMS]");
		return;
#endif
	}

	char* OrigFilename = argv[0];
	PathStripPath(OrigFilename);

	char RestOfFilename[MAX_EXE_NAME];
	StrCpyN(RestOfFilename, OrigFilename, (int)(StrChr(OrigFilename, '.') - OrigFilename) + 1);

	//char OldFilename[MAX_EXE_NAME];
	//char NewFilename[MAX_EXE_NAME];
	//StrCpy(OldFilename, OrigFilename);
	//int Iter = 0;
	//char* OldFilePtr = OldFilename, * NewFilePtr = NewFilename;
	//while (Iter < ProcCount)
	//{
	//	sprintf(NewFilePtr, "%s%i.exe", RestOfFilename, Iter);
	//	printf("Moving %s, %s\n", OldFilePtr, NewFilePtr);
	//	if (!MoveFile(OldFilePtr, NewFilePtr))
	//	{
	//		printf("AAAHH COULDNT MOVE, %i\n", GetLastError());
	//		getchar();
	//		return;
	//	}
	//	Iter++;
	//
	//	char* Tmp = OldFilePtr;
	//	OldFilePtr = NewFilePtr;
	//	NewFilePtr = Tmp;
	//
	//	if (SleepMS)
	//	{
	//		Sleep(SleepMS);
	//	}
	//}

	if (SleepMS)
	{
		Sleep(SleepMS);
	}

	char NewFilename[MAX_EXE_NAME];
	sprintf(NewFilename, "%s%i.exe", RestOfFilename, 0);
	CopyFile(OrigFilename, NewFilename, true);

	HANDLE NextProc;
	int delay = 10;
	while ((NextProc = CreateFile (NewFilename, GENERIC_EXECUTE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_SHARING_VIOLATION) {
			Sleep (delay);
			if (delay < 5120) // max delay approx 5.Sec
				delay *= 2;
		}
		else
			break; // some other error occurred
	}

	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;
	assert(CreateProcess(NewFilename, 0, 0, 0, false, 0, 0, 0, &StartupInfo, &ProcInfo));


}