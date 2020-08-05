#pragma region UTF
void WidenUTF(wchar_t *Dst, i32 DstLen, const char *Src, i32 SrcLen = -1)
{
	if(SrcLen == -1)
	{
		SrcLen = (i32)strlen(Src);
	}
	SrcLen++;
	assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Src, SrcLen, Dst, DstLen));
}

wchar_t *WidenUTFAlloc(const char *Str, i32 StrLen = -1)
{
	if(StrLen == -1)
	{
		StrLen = (i32)strlen(Str);
	}
	StrLen++;
	i32 ReqSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Str, StrLen, 0, 0);
	wchar_t *Res = (wchar_t *)malloc(ReqSize);
	WidenUTF(Res, ReqSize, Str, StrLen);
	return Res;
}

void NarrowUTF(char *Dst, i32 DstLen, const wchar_t *Src, i32 SrcLen = -1)
{
	if(SrcLen == -1)
	{
		SrcLen = (i32)wcslen(Src);
	}
	SrcLen++;
	assert(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Src, SrcLen, Dst, DstLen, 0, 0));
}

char *NarrowUTFAlloc(const wchar_t *Str, i32 StrLen = -1)
{
	if(StrLen == -1)
	{
		StrLen = (i32)wcslen(Str);
	}
	StrLen++;
	i32 ReqSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Str, StrLen, 0, 0, 0, 0);
	char *Res = (char *)malloc(ReqSize);
	NarrowUTF(Res, ReqSize, Str, StrLen);
	return Res;
}

#define FreeUTF(Ptr) free((Ptr));
#pragma endregion


typedef char pchar;
typedef char ichar;

/*
	No functions are specified for ANSI and Wide, since literals can be
	used directly ('L' for Wide, none for ANSI).
*/
#define PCHAR(x) x
#define ICHAR(x) x

#define asprintf(...) sprintf_s(__VA_ARGS__)
#define wsprintf(...) swprintf_s(__VA_ARGS__)
#define isprintf(...) sprintf_s(__VA_ARGS__)
#define psprintf(...) sprintf_s(__VA_ARGS__)

internal void
DebugLogA(const char *Str)
{
	OutputDebugStringA(Str);
}

internal void
DebugLogW(const wchar_t *Str)
{
	OutputDebugStringW(Str);
}

internal void
DebugLogI(const ichar *Str)
{
	DebugLogA(Str);
}

internal void
DebugLogP(const pchar *Str)
{
	DebugLogA(Str);
}