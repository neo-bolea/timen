//#ifndef VS_DUMMY
//#define DEBUG
//#define UNICODE
//#endif
//
//#include <stdint.h>
//
//typedef int8_t i8;
//typedef int16_t i16;
//typedef int32_t i32;
//typedef int64_t i64;
//
//typedef uint8_t ui8;
//typedef uint16_t ui16;
//typedef uint32_t ui32;
//typedef uint64_t ui64;
//
//typedef float f32;
//typedef double f64;
//
//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
//
//#ifdef DEBUG
//#define assert(Expr) \
//	if(!(Expr))        \
//	{                  \
//		*(i32 *)0 = 0;   \
//	}                  \
//	0
//#else
//#define assert(Expr) \
//	(Expr);            \
//	0
//#endif
//
//
//
//#define ID_FILE_EXIT 9001
//#define ID_STUFF_GO 9002
//#define ID_BUTTON_OK 9003
//
//LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
//
//int __stdcall WinMain(
//		HINSTANCE hInstance,
//		HINSTANCE hPrevInstance,
//		LPSTR lpCmdLine,
//		int nShowCmd)
//{
//	WNDCLASSEX WndClass;
//	memset(&WndClass, 0, sizeof(WNDCLASSEX));
//	WndClass.cbSize = sizeof(WNDCLASSEX);
//	WndClass.lpfnWndProc = &WndProc;
//	WndClass.hbrBackground = CreateSolidBrush(0x00101010);
//	WndClass.hInstance = hInstance;
//	WndClass.lpszClassName = L"timen";
//	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
//	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
//	assert(RegisterClassEx(&WndClass));
//	//	SetWindowLongPtr(Window, GWL_STYLE, sc.WindowedStyle_ & ~WS_OVERLAPPEDWINDOW);
//	//SetWindowLongPtr(Window, GWL_EXSTYLE,
//	//								 sc.WindowedExStyle_ & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
//	HWND Window = CreateWindow(WndClass.lpszClassName, L"timen", WS_POPUP | WS_VISIBLE | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, 0, 0, hInstance, 0);
//	assert(Window != INVALID_HANDLE_VALUE);
//
//	HWND WndButton = CreateWindowEx(NULL,
//																	L"BUTTON",
//																	L"ok",
//																	WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
//																	0,
//																	0,
//																	100,
//																	24,
//																	Window,
//																	(HMENU)ID_BUTTON_OK,
//																	GetModuleHandle(NULL),
//																	NULL);
//	assert(WndButton != INVALID_HANDLE_VALUE);
//	//SendMessage(WndButton, );
//
//	ShowWindow(Window, true);
//
//	MSG Msg;
//	while(GetMessage(&Msg, 0, 0, 0))
//	{
//		TranslateMessage(&Msg);
//		DispatchMessageW(&Msg);
//	}
//}
//
//void Marker(LONG x, LONG y, HWND hwnd)
//{
//	HDC hdc;
//
//	hdc = GetDC(hwnd);
//	MoveToEx(hdc, (int)x - 10, (int)y, (LPPOINT)NULL);
//	LineTo(hdc, (int)x + 10, (int)y);
//	MoveToEx(hdc, (int)x, (int)y - 10, (LPPOINT)NULL);
//	LineTo(hdc, (int)x, (int)y + 10);
//
//	ReleaseDC(hwnd, hdc);
//}
//
//static POINT ptMouseDown[32];
//static int index;
//POINTS ptTmp;
//RECT rc;
//
//LRESULT CALLBACK WndProc(HWND Wnd, UINT Message, WPARAM wParam, LPARAM lParam)
//{
//	LRESULT Result = 0;
//	switch(Message)
//	{
//		case WM_CREATE:
//		{
//			HMENU hMenu, hSubMenu;
//
//			hMenu = CreateMenu();
//
//			// TODO: Use MF_OWNERDRAW
//			hSubMenu = CreatePopupMenu();
//			AppendMenu(hSubMenu, MF_STRING, ID_FILE_EXIT, L"E&xit");
//			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&File");
//
//			hSubMenu = CreatePopupMenu();
//			AppendMenu(hSubMenu, MF_STRING, ID_STUFF_GO, L"&Go");
//			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"&Stuff");
//
//			SetMenu(Wnd, hMenu);
//		}
//		break;
//		case WM_COMMAND:
//			switch(LOWORD(wParam))
//			{
//				case ID_FILE_EXIT:
//					PostQuitMessage(0);
//					break;
//				case ID_STUFF_GO:
//
//					break;
//			}
//			break;
//		//case WM_DRAWITEM:
//		//{
//		//	DRAWITEMSTRUCT &DrawInfo = *(DRAWITEMSTRUCT *)lParam;
//		//	HDC mDC = CreateCompatibleDC(DrawInfo.hDC);
//		//	if(DrawInfo.CtlID == ID_BUTTON_OK)
//		//	{
//		//		if(DrawInfo.itemState & ODS_FOCUS)
//		//		{
//		//		}
//		//		else
//		//		{
//		//		}
//		//
//		//		RECT *Rect = &DrawInfo.rcItem;
//		//		Rect->right += 1000;
//		//		BitBlt(mDC, Rect->left, Rect->top, Rect->right - Rect->left, Rect->bottom - Rect->top, mDC, 0, 0, SRCCOPY);
//		//		//assert(FillRect(mDC, Rect, CreateSolidBrush(0x00FF0000)));
//		//
//		//		Result = true;
//		//	}
//		//	else
//		//	{
//		//		Result = DefWindowProc(Wnd, Message, wParam, lParam);
//		//	}
//		//	DeleteDC(mDC);
//		//}
//		//break;
//
//		case WM_DESTROY:
//			PostQuitMessage(0);
//			break;
//
//		case WM_LBUTTONDOWN:
//			if(index < 32)
//			{
//				// Create the region from the client area.
//				GetClientRect(Wnd, &rc);
//				HRGN hrgn = CreateRectRgn(rc.left, rc.top,
//																	rc.right, rc.bottom);
//
//				ptTmp = MAKEPOINTS(lParam);
//				ptMouseDown[index].x = (LONG)ptTmp.x;
//				ptMouseDown[index].y = (LONG)ptTmp.y;
//
//				// Test for a hit in the client rectangle.
//
//				if(PtInRegion(hrgn, ptMouseDown[index].x,
//											ptMouseDown[index].y))
//				{
//					// If a hit occurs, record the mouse coords.
//
//					Marker(ptMouseDown[index].x, ptMouseDown[index].y,
//								 Wnd);
//					index++;
//				}
//
//				DeleteObject(hrgn);
//			}
//			break;
//
//		case WM_NCHITTEST:
//		{
//			Result = DefWindowProc(Wnd, Message, wParam, lParam);
//			return (Result == HTMENU) ? HTCAPTION : Result;
//		}
//		break;
//
//		default:
//			Result = DefWindowProc(Wnd, Message, wParam, lParam);
//	}
//	return Result;
//}
#ifndef UNICODE
#define UNICODE
#endif

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <Windows.h>
#include <Commctrl.h>

#define IDC_EXIT_BUTTON 101
#define IDC_PUSHLIKE_BUTTON 102

HBRUSH CreateGradientBrush(COLORREF top, COLORREF bottom, LPNMCUSTOMDRAW item)
{
	HBRUSH Brush = NULL;
	HDC hdcmem = CreateCompatibleDC(item->hdc);
	HBITMAP hbitmap = CreateCompatibleBitmap(item->hdc, item->rc.right - item->rc.left, item->rc.bottom - item->rc.top);
	SelectObject(hdcmem, hbitmap);

	int r1 = GetRValue(top), r2 = GetRValue(bottom), g1 = GetGValue(top), g2 = GetGValue(bottom), b1 = GetBValue(top), b2 = GetBValue(bottom);
	for(int i = 0; i < item->rc.bottom - item->rc.top; i++)
	{
		RECT temp;
		int r, g, b;
		r = int(r1 + double(i * (r2 - r1) / item->rc.bottom - item->rc.top));
		g = int(g1 + double(i * (g2 - g1) / item->rc.bottom - item->rc.top));
		b = int(b1 + double(i * (b2 - b1) / item->rc.bottom - item->rc.top));
		Brush = CreateSolidBrush(RGB(r, g, b));
		temp.left = 0;
		temp.top = i;
		temp.right = item->rc.right - item->rc.left;
		temp.bottom = i + 1;
		FillRect(hdcmem, &temp, Brush);
		DeleteObject(Brush);
	}
	HBRUSH pattern = CreatePatternBrush(hbitmap);

	DeleteDC(hdcmem);
	DeleteObject(Brush);
	DeleteObject(hbitmap);

	return pattern;
}

bool Running = true;

LRESULT CALLBACK MainWindow(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH defaultbrush = NULL;
	static HBRUSH hotbrush = NULL;
	static HBRUSH selectbrush = NULL;
	static HBRUSH push_uncheckedbrush = NULL;
	static HBRUSH push_checkedbrush = NULL;
	static HBRUSH push_hotbrush1 = NULL;
	static HBRUSH push_hotbrush2 = NULL;
	switch(msg)
	{
		case WM_CREATE:
		{
			HWND Exit_Button = CreateWindowEx(NULL, L"BUTTON", L"EXIT",
																				WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
																				50, 50, 100, 100, hwnd, (HMENU)IDC_EXIT_BUTTON, NULL, NULL);
			if(Exit_Button == NULL)
			{
				MessageBox(NULL, L"Button Creation Failed!", L"Error!", MB_ICONEXCLAMATION);
				exit(EXIT_FAILURE);
			}

			HWND Pushlike_Button = CreateWindowEx(NULL, L"BUTTON", L"PUSH ME!",
																						WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHLIKE,
																						200, 50, 100, 100, hwnd, (HMENU)IDC_PUSHLIKE_BUTTON, NULL, NULL);
			if(Pushlike_Button == NULL)
			{
				MessageBox(NULL, L"Button Creation Failed!", L"Error!", MB_ICONEXCLAMATION);
				exit(EXIT_FAILURE);
			}
		}
		break;
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_EXIT_BUTTON:
				{
					SendMessage(hwnd, WM_CLOSE, 0, 0);
					Running = false;
				}
				break;
			}
		}
		break;
		case WM_NOTIFY:
		{
			LPNMHDR some_item = (LPNMHDR)lParam;

			if(some_item->idFrom == IDC_EXIT_BUTTON && some_item->code == NM_CUSTOMDRAW)
			{
				LPNMCUSTOMDRAW item = (LPNMCUSTOMDRAW)some_item;

				if(item->uItemState & CDIS_SELECTED)
				{
					//Select our color when the button is selected
					if(selectbrush == NULL)
						selectbrush = CreateGradientBrush(RGB(180, 0, 0), RGB(255, 180, 0), item);

					//Create pen for button border
					HPEN pen = CreatePen(PS_INSIDEFRAME, 0, RGB(0, 0, 0));

					//Select our brush into hDC
					HGDIOBJ old_pen = SelectObject(item->hdc, pen);
					HGDIOBJ old_brush = SelectObject(item->hdc, selectbrush);

					//If you want rounded button, then use this, otherwise use FillRect().
					RoundRect(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom, 5, 5);

					//Clean up
					SelectObject(item->hdc, old_pen);
					SelectObject(item->hdc, old_brush);
					DeleteObject(pen);

					//Now, I don't want to do anything else myself (draw text) so I use this value for return:
					return CDRF_DODEFAULT;
					//Let's say I wanted to draw text and stuff, then I would have to do it before return with
					//DrawText() or other function and return CDRF_SKIPDEFAULT
				}
				else
				{
					if(item->uItemState & CDIS_HOT) //Our mouse is over the button
					{
						//Select our color when the mouse hovers our button
						if(hotbrush == NULL)
							hotbrush = CreateGradientBrush(RGB(255, 230, 0), RGB(245, 0, 0), item);

						HPEN pen = CreatePen(PS_INSIDEFRAME, 0, RGB(0, 0, 0));

						HGDIOBJ old_pen = SelectObject(item->hdc, pen);
						HGDIOBJ old_brush = SelectObject(item->hdc, hotbrush);

						RoundRect(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom, 5, 5);

						SelectObject(item->hdc, old_pen);
						SelectObject(item->hdc, old_brush);
						DeleteObject(pen);

						return CDRF_DODEFAULT;
					}

					//Select our color when our button is doing nothing
					if(defaultbrush == NULL)
						defaultbrush = CreateGradientBrush(RGB(255, 180, 0), RGB(180, 0, 0), item);

					HPEN pen = CreatePen(PS_INSIDEFRAME, 0, RGB(0, 0, 0));

					HGDIOBJ old_pen = SelectObject(item->hdc, pen);
					HGDIOBJ old_brush = SelectObject(item->hdc, defaultbrush);

					RoundRect(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom, 5, 5);

					SelectObject(item->hdc, old_pen);
					SelectObject(item->hdc, old_brush);
					DeleteObject(pen);

					return CDRF_DODEFAULT;
				}
			}
			else if(some_item->idFrom == IDC_PUSHLIKE_BUTTON && some_item->code == NM_CUSTOMDRAW)
			{
				LPNMCUSTOMDRAW item = (LPNMCUSTOMDRAW)some_item;

				if(IsDlgButtonChecked(hwnd, (int)some_item->idFrom))
				{
					if(item->uItemState & CDIS_HOT)
					{
						if(push_hotbrush1 == NULL)
							push_hotbrush1 = CreateGradientBrush(RGB(0, 0, 245), RGB(0, 230, 255), item);

						HPEN pen = CreatePen(PS_INSIDEFRAME, 0, RGB(0, 0, 0));

						HGDIOBJ old_pen = SelectObject(item->hdc, pen);
						HGDIOBJ old_brush = SelectObject(item->hdc, push_hotbrush1);

						RoundRect(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom, 10, 10);

						SelectObject(item->hdc, old_pen);
						SelectObject(item->hdc, old_brush);
						DeleteObject(pen);

						return CDRF_DODEFAULT;
					}


					if(push_checkedbrush == NULL)
						push_checkedbrush = CreateGradientBrush(RGB(0, 0, 180), RGB(0, 222, 200), item);


					HPEN pen = CreatePen(PS_INSIDEFRAME, 0, RGB(0, 0, 0));


					HGDIOBJ old_pen = SelectObject(item->hdc, pen);
					HGDIOBJ old_brush = SelectObject(item->hdc, push_checkedbrush);


					RoundRect(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom, 10, 10);


					SelectObject(item->hdc, old_pen);
					SelectObject(item->hdc, old_brush);
					DeleteObject(pen);


					return CDRF_DODEFAULT;
				}
				else
				{
					if(item->uItemState & CDIS_HOT)
					{
						if(push_hotbrush2 == NULL)
							push_hotbrush2 = CreateGradientBrush(RGB(255, 230, 0), RGB(245, 0, 0), item);

						HPEN pen = CreatePen(PS_INSIDEFRAME, 0, RGB(0, 0, 0));

						HGDIOBJ old_pen = SelectObject(item->hdc, pen);
						HGDIOBJ old_brush = SelectObject(item->hdc, push_hotbrush2);

						RoundRect(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom, 10, 10);

						SelectObject(item->hdc, old_pen);
						SelectObject(item->hdc, old_brush);
						DeleteObject(pen);

						return CDRF_DODEFAULT;
					}

					if(push_uncheckedbrush == NULL)
						push_uncheckedbrush = CreateGradientBrush(RGB(255, 180, 0), RGB(180, 0, 0), item);

					HPEN pen = CreatePen(PS_INSIDEFRAME, 0, RGB(0, 0, 0));

					HGDIOBJ old_pen = SelectObject(item->hdc, pen);
					HGDIOBJ old_brush = SelectObject(item->hdc, defaultbrush);

					RoundRect(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom, 10, 10);

					SelectObject(item->hdc, old_pen);
					SelectObject(item->hdc, old_brush);
					DeleteObject(pen);

					return CDRF_DODEFAULT;
				}
			}
			return CDRF_DODEFAULT;
		}
		break;
		case WM_CTLCOLORBTN: //In order to make those edges invisble when we use RoundRect(),
		{ //we make the color of our button's background match window's background
			return (LRESULT)GetSysColorBrush(COLOR_WINDOW + 1);
		}
		break;
		case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			return 0;
		}
		break;
		case WM_DESTROY:
		{
			DeleteObject(defaultbrush);
			DeleteObject(selectbrush);
			DeleteObject(hotbrush);
			DeleteObject(push_checkedbrush);
			DeleteObject(push_hotbrush1);
			DeleteObject(push_hotbrush2);
			DeleteObject(push_uncheckedbrush);
			PostQuitMessage(0);
			return 0;
		}
		break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG msg = {};
	const wchar_t ClassName[] = L"Main_Window";

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = MainWindow;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = ClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
		exit(EXIT_FAILURE);
	}

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, ClassName, L"Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 368, 248, NULL, NULL, hInstance, NULL);

	if(hwnd == NULL)
	{
		MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		exit(EXIT_FAILURE);
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while(Running)
	{
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		Sleep(800);
		ShowWindow(hwnd, SW_HIDE);
		Sleep(200);
		ShowWindow(hwnd, SW_SHOW);
		SetForegroundWindow(hwnd);
	}

	return msg.message;
}