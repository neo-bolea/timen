#ifndef VS_DUMMY
#define DEBUG
#define UNICODE
#endif

#include <stdint.h>

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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef DEBUG
#define assert(Expr) \
	if(!(Expr))        \
	{                  \
		*(i32 *)0 = 0;   \
	}                  \
	0
#else
#define assert(Expr) \
	(Expr);            \
	0
#endif

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

int __stdcall WinMain(
		HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nShowCmd)
{
	WNDCLASSEX WndClass;
	memset(&WndClass, 0, sizeof(WNDCLASSEX));
	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.lpfnWndProc = &WndProc;
	WndClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	WndClass.hInstance = hInstance;
	WndClass.lpszClassName = L"timen";
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	assert(RegisterClassEx(&WndClass));
	//	SetWindowLongPtr(Window, GWL_STYLE, sc.WindowedStyle_ & ~WS_OVERLAPPEDWINDOW);
	//SetWindowLongPtr(Window, GWL_EXSTYLE,
	//								 sc.WindowedExStyle_ & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
	HWND Window = CreateWindow(WndClass.lpszClassName, L"timen", WS_POPUP | WS_VISIBLE | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, 0, 0, hInstance, 0);
	assert(Window != INVALID_HANDLE_VALUE);

	ShowWindow(Window, true);

	MSG Msg;
	while(GetMessage(&Msg, 0, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessageW(&Msg);
	}
}

void Marker(LONG x, LONG y, HWND hwnd)
{
	HDC hdc;

	hdc = GetDC(hwnd);
	MoveToEx(hdc, (int)x - 10, (int)y, (LPPOINT)NULL);
	LineTo(hdc, (int)x + 10, (int)y);
	MoveToEx(hdc, (int)x, (int)y - 10, (LPPOINT)NULL);
	LineTo(hdc, (int)x, (int)y + 10);

	ReleaseDC(hwnd, hdc);
}

static POINT ptMouseDown[32];
static int index;
POINTS ptTmp;
RECT rc;

#define ID_FILE_EXIT 9001
#define ID_STUFF_GO 9002
LRESULT CALLBACK WndProc(HWND Wnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;
	switch(Message)
	{
		case WM_CREATE:
		{
			HMENU hMenu, hSubMenu;

			hMenu = CreateMenu();

			hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, ID_FILE_EXIT, L"E&xit");
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&File");

			hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, ID_STUFF_GO, L"&Go");
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Stuff");

			SetMenu(Wnd, hMenu);
		}
		break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_FILE_EXIT:
					PostQuitMessage(0);
					break;
				case ID_STUFF_GO:

					break;
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_LBUTTONDOWN:
			if(index < 32)
			{
				// Create the region from the client area.
				GetClientRect(Wnd, &rc);
				HRGN hrgn = CreateRectRgn(rc.left, rc.top,
																	rc.right, rc.bottom);

				ptTmp = MAKEPOINTS(lParam);
				ptMouseDown[index].x = (LONG)ptTmp.x;
				ptMouseDown[index].y = (LONG)ptTmp.y;

				// Test for a hit in the client rectangle.

				if(PtInRegion(hrgn, ptMouseDown[index].x,
											ptMouseDown[index].y))
				{
					// If a hit occurs, record the mouse coords.

					Marker(ptMouseDown[index].x, ptMouseDown[index].y,
								 Wnd);
					index++;
				}

				DeleteObject(hrgn);
			}
			break;

		case WM_NCHITTEST:
		{
			Result = DefWindowProc(Wnd, Message, wParam, lParam);
			return (Result == HTMENU) ? HTCAPTION : Result;
		}
		break;

		default:
			Result = DefWindowProc(Wnd, Message, wParam, lParam);
	}
	return Result;
}