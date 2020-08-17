#pragma region String Helpers
internal void
StrECpy(char *Dst, const char *Src, char EndChar)
{
	while(*Src && *Src != EndChar)
	{
		*Dst++ = *Src++;
	}
}

internal ui32
StrCCount(const char *Str, char C)
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

internal char *
StrEnd(char *Str)
{
	while(*Str)
	{
		Str++;
	}
	return Str;
}

internal char *
StrStrRFind(char *Str, const char *SubStr, size_t StrLen = (size_t)-1, size_t SubStrLen = (size_t)-1)
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

internal void
WidenUTF(wchar_t *Dst, i32 DstLen, const char *Src)
{
	Assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Src, -1, Dst, DstLen));
}

internal wchar_t *
WidenUTFAlloc(const char *Str)
{
	i32 ReqSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Str, -1, 0, 0);
	wchar_t *Res = (wchar_t *)malloc(ReqSize);
	WidenUTF(Res, ReqSize, Str);
	return Res;
}

internal void
NarrowUTF(char *Dst, i32 DstLen, wchar_t *Src)
{
	Assert(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Src, -1, Dst, DstLen, 0, 0));
}

internal char *
NarrowUTFAlloc(wchar_t *Str)
{
	i32 ReqSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Str, -1, 0, 0, 0, 0);
	char *Res = (char *)malloc(ReqSize);
	NarrowUTF(Res, ReqSize, Str);
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
global const char *EOLStr = "\n"; // TODO NOW: Change back to comma or other non-line symbol once parsing is correct (title commas are also parsed!)
global const ui32 EOLLen = (ui32)strlen(EOLStr);

internal bool
IsEOLChar(char C)
{
	for(size_t i = 0; i < EOLLen; i++)
	{
		if(C == EOLStr[i])
		{
			return true;
		}
	}
	return false;
}

// Compares strings until EOL or end of string
// EOLStr is the string that contains the new line character, Str is assumed to have no new line
internal bool
StrCmpEOL(const char *MulLineStr, const char *Str)
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

internal ui32
EOLCount(const char *Str)
{
	ui32 Count = 0;
	while(*Str)
	{
		Count += IsEOLChar(*Str++);
	}
	return Count;
}

internal char *
AfterNextEOL(char *Str)
{
	if(!Str)
	{
		return 0;
	}
	do
	{
		if(*Str == EOLStr[0])
		{
			return Str + EOLLen;
		}
	} while(*Str++);
	return 0;
}

internal char *
InsertEOL(char *Str)
{
	for(size_t i = 0; i < EOLLen; i++)
	{
		*Str++ = EOLStr[i];
	}
	return Str;
}
#pragma endregion

#pragma region Debug Logging
HANDLE DebugConsole;
internal void
LogV(ui8 Color, const char *Format, va_list Args)
{
	SetConsoleTextAttribute(DebugConsole, Color);
	vprintf(Format, Args);
	SetConsoleTextAttribute(DebugConsole, 0x07);
}

internal void
Log(const char *Format...)
{
	va_list Args;
	va_start(Args, Format);
	LogV(0x07, Format, Args);
	va_end(Args);
}

internal void
Log(ui8 Color, const char *Format...)
{
	va_list Args;
	va_start(Args, Format);
	LogV(Color, Format, Args);
	va_end(Args);
}
#pragma endregion