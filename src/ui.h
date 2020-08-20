typedef struct time_graph_data
{
	uint VAO, VBO, CalProg;
	uint QuadVAO, FBProg;
	t_min Center;
	ui64 Zoom;
	uint ScrollSensitivity;
} time_graph_data;

typedef struct win_data
{
	NOTIFYICONDATA *NotifIcon;
	COLORREF BGColor;
	HWND TimeGraphWnd;
	time_graph_data TimeGraphData;
	timen_cfg *tcfg;
} win_data;

void UpdateWindowSize(win_data &WD, HWND Wnd, WPARAM W, LPARAM L)
{
	if(WD.TimeGraphWnd)
	{
		int PosX = 50, PosY = 50;
		int Width = (LOWORD(L) - PosX) - 50, Height = (HIWORD(L) - PosY) - 50;

		if(Width > 0 && Height > 0)
		{
			SetWindowPos(WD.TimeGraphWnd, 0, 0, 0, Width, Height, SWP_NOMOVE);
		}
	}
}

LRESULT CALLBACK TimeMainGraphProc(HWND Wnd, UINT Msg, WPARAM W, LPARAM L);
LRESULT CALLBACK WndProc(HWND Wnd, UINT Msg, WPARAM W, LPARAM L);

#include "gl_graphics.h"

#define WM_USER_SHELLICON WM_USER + 1
#define IDM_EXIT WM_USER + 2
#define IDM_OPEN WM_USER + 3
DWORD WINAPI MainWindow(void *Args)
{
	// TODO: Use manifest file instead, as msdn suggests (https://docs.microsoft.com/en-us/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process)?
	Assert(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));

	HINSTANCE hInstance = GetModuleHandle(NULL);
	timen_cfg &tcfg = *(timen_cfg *)Args;

	win_data WD = {};
	WD.tcfg = &tcfg;
	HWND Window, TimeMainGraph;
	NOTIFYICONDATA NotifIcon;
	{
		WD.BGColor = RGB(0, 80, 120);

		{
			WNDCLASSEX WndClass = {sizeof(WNDCLASSEX)};
			WndClass.lpfnWndProc = &WndProc;
			WndClass.hInstance = hInstance;
			WndClass.lpszClassName = L"timen";
			WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
			WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
			WndClass.style = CS_OWNDC;
			WndClass.hbrBackground = CreateSolidBrush(WD.BGColor);
			Assert(RegisterClassEx(&WndClass));
			Window = CreateWindow(WndClass.lpszClassName, L"timen", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
			Assert(Window != INVALID_HANDLE_VALUE);
			SetWindowLongPtr(Window, GWLP_USERDATA, (LONG_PTR)&WD);
		}

		{
			WNDCLASS WndClass = {};
			WndClass.lpfnWndProc = &TimeMainGraphProc;
			WndClass.hInstance = hInstance;
			WndClass.lpszClassName = L"time graph";
			WndClass.style = CS_OWNDC;
			Assert(RegisterClass(&WndClass));
			TimeMainGraph = CreateWindow(WndClass.lpszClassName, L"time graph", WS_CHILD | WS_VISIBLE, 50, 50, CW_USEDEFAULT, CW_USEDEFAULT, Window, 0, hInstance, 0);
			Assert(TimeMainGraph != INVALID_HANDLE_VALUE);
			SetWindowLongPtr(TimeMainGraph, GWLP_USERDATA, (LONG_PTR)&WD);
			WD.TimeGraphWnd = TimeMainGraph;

			RECT WinRect;
			GetClientRect(Window, &WinRect);
			UpdateWindowSize(WD, Window, SIZE_RESTORED, MAKELPARAM(WinRect.right - WinRect.left, WinRect.bottom - WinRect.top));
		}

		WD.NotifIcon = &NotifIcon;
		NotifIcon = {sizeof(NOTIFYICONDATA)};
		NotifIcon.hWnd = Window;
		NotifIcon.uID = 0;
		NotifIcon.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		NotifIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		NotifIcon.uCallbackMessage = WM_USER_SHELLICON;
		wcscpy_s(NotifIcon.szTip, ArrLen(NotifIcon.szTip), L"timen");
		Assert(Shell_NotifyIcon(NIM_ADD, &NotifIcon));
	}

	{
		time_graph_data &GraphData = WD.TimeGraphData;
		FILETIME FTNow;
		GetSystemTimeAsFileTime(&FTNow);
		GraphData.Center = 19985800; //(ULARGE_INTEGER{FTNow.dwLowDateTime, FTNow.dwHighDateTime}.QuadPart - tcfg.GlobalTimeBegin) / TMinToTFile;
		GraphData.Zoom = 6;
		GraphData.ScrollSensitivity = 48;
	}

#ifdef DEBUG
	ShowWindow(Window, true);
#endif

	MSG Msg;
	while(GetMessage(&Msg, Window, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &NotifIcon);
	DestroyWindow(Window);

	Running = false;
	return 0;
}

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
					InsertMenu(hPopMenu, (ui32)-1, MF_BYPOSITION | MF_STRING, IDM_OPEN, L"Open");
					InsertMenu(hPopMenu, (ui32)-1, MF_BYPOSITION | MF_STRING, IDM_EXIT, L"Exit");
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
				case IDM_OPEN:
					ShowWindow(Wnd, true);
					break;
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


		// TODO: Test if DPICHANGED is handled correctly by using multiple monitors.
		// TODO: Follow tutorial on https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows and https://github.com/tringi/win32-dpi
		case WM_DPICHANGED:
		case WM_SIZE:
		{
			UpdateWindowSize(*WD, Wnd, W, L);

			// TODO: Fix for all DPIs.
			HMONITOR Monitor = MonitorFromWindow(Wnd, MONITOR_DEFAULTTONEAREST);
			DEVICE_SCALE_FACTOR ScaleFactor;
			GetScaleFactorForMonitor(Monitor, &ScaleFactor);
			f32 DPIScale = ScaleFactor / 100.0f;

			uint Width = (uint)(LOWORD(L) * DPIScale), Height = (uint)(HIWORD(L) * DPIScale);
			UpdateFramebuffers(Width, Height);
			glViewport(0, 0, Width, Height);
			RedrawWindow(Wnd, 0, 0, RDW_INVALIDATE);
			Result = DefWindowProc(Wnd, Msg, W, L);
		}
		break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			Result = TimeMainGraphProc(Wnd, Msg, W, L);
			break;

		default:
			Result = DefWindowProc(Wnd, Msg, W, L);
	}

	return Result;
}

LRESULT CALLBACK TimeMainGraphProc(HWND Wnd, UINT Msg, WPARAM W, LPARAM L)
{
	LRESULT Result = 0;
	win_data *WD = (win_data *)GetWindowLongPtr(Wnd, GWLP_USERDATA);

	switch(Msg)
	{
		case WM_CREATE:
		{
			InitOpenGL(GetDC(Wnd));
		}
		break;

		// TODO: Use smooth, continuous movement instead of autorepeat?
		case WM_KEYDOWN:
		{
			time_graph_data &tgd = WD->TimeGraphData;
			timen_cfg &tcfg = *WD->tcfg;

			bool Down = (Msg == WM_KEYDOWN);
			ui64 ScrollAmount = MIN(MAX(tgd.Zoom / tgd.ScrollSensitivity, 1) * tcfg.DivTime, (ui64)tgd.Center);

			if(W == 'A' || W == VK_LEFT)
			{
				tgd.Center -= ScrollAmount;
			}
			else if(W == 'D' || W == VK_RIGHT)
			{
				tgd.Center += ScrollAmount;
			}
			RedrawWindow(Wnd, 0, 0, RDW_INVALIDATE);
		}
		break;
		case WM_MOUSEWHEEL:
		{
			time_graph_data &tgd = WD->TimeGraphData;
			timen_cfg &tcfg = *WD->tcfg;

			i16 Scroll = GET_WHEEL_DELTA_WPARAM(W) / WHEEL_DELTA;
			tgd.Zoom -= Scroll;
			tgd.Zoom = MAX(tgd.Zoom, 1);
			RedrawWindow(Wnd, 0, 0, RDW_INVALIDATE);
		}
		break;
		// TODO: Paint live while logging, if the window is active
		// TODO: Refactor to unit test
		// TODO: Go again over this whole mess
		case WM_PAINT:
		{
			time_graph_data &tgd = WD->TimeGraphData;
			timen_cfg &tcfg = *WD->tcfg;

			t_min *ActivityTimes = 0;
			f32 *NormActTimes = 0;

			PAINTSTRUCT PS;
			HDC DC = BeginPaint(WD->TimeGraphWnd, &PS);

			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glDisable(GL_DEPTH);

			t_min StartDate = MAX((t_min_d)tgd.Center - (t_min_d)(tgd.Zoom * tcfg.DivTime), 0);
			StartDate = (StartDate / tcfg.DivTime) * tcfg.DivTime;
			t_min EndDate = tgd.Center + tgd.Zoom * tcfg.DivTime;
			EndDate = (EndDate / tcfg.DivTime + 1) * tcfg.DivTime;

			t_div TotalHours = (t_div)(EndDate / tcfg.DivTime - StartDate / tcfg.DivTime);
			Assert(TotalHours < 65536);
			TotalHours = MIN(TotalHours, 65536);

			// Find the range of the file where the activities to collect are.
			// TODO NOW: Some type of read/write protection or other mechanism, since main thread already writes to the file.
			ui64 StpFileSize;
			char *StpBuf = (char *)LoadFileContents(tcfg.StampFilename, &StpFileSize);
			void *LogFile = OpenFile(tcfg.LogFilename, FA_Read, FS_All);
			ui64 LogFileSize = GetFileSize32(LogFile);
			// TODO: Show the whole time range, not only the bounding range of the valid values!
			// Includes all timestamps in the requested range, but also the two timestamps before and after.
			stamp_range RangeStamps = GetTimeStampsInRange(StpBuf, StpFileSize, LogFileSize, StartDate, EndDate);

			// TODO: Instead of reserving memory for all programs, how about instead inserting to some type of map and allocating dynamically?
			const ui32 TotalProgs = GetProcSymCount(tcfg);

			uint ActivityLen = (uint)(TotalProgs * TotalHours);
			ActivityTimes = (t_min *)malloc(ActivityLen * sizeof(t_min));
			memset(ActivityTimes, 0, ActivityLen * sizeof(t_min));

			// Process the activities to make presentable data out of them.
			if(RangeStamps.Size)
			{
				byte_mark StartMarker = RangeStamps[0].Marker, EndMarker = RangeStamps[RangeStamps.Size - 1].Marker;
				size_t FileRangeSize = (size_t)(EndMarker - StartMarker);

				if(FileRangeSize)
				{
					// There must be at least two timestamps, since otherwise this doesn't represent a range.
					Assert(RangeStamps.Size >= 2);

					// Start date must be before the second stamp, since the stamp is included in the range (other way around for end date).
					Assert(StartDate <= RangeStamps[1].Time);
					Assert(RangeStamps[RangeStamps.Size - 2].Time <= EndDate);

					char *LogBuf;
					{
						ui64 FilePointer = StartMarker;
						Assert(SetFilePointerEx(LogFile, *(LARGE_INTEGER *)&FilePointer, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER);
						file_result FileResult;
						LogBuf = (char *)ReadFileContents(LogFile, (ui32)FileRangeSize, &FileResult);
						CloseFile(LogFile);
						Assert(FileResult == file_result::Success);
					}
					char *LogPos = LogBuf;

					t_min_d TimeUntilStartDate = (StartDate - RangeStamps[0].Time);
					byte_mark BytesUntilSecStamp = RangeStamps[1].Marker - RangeStamps[0].Marker;

					i32 ASym = INT32_MAX;
					t_min ATime = 0;
					// Move forward until we reach the date at which we want to start (we know that the start date lies between the 1st and 2nd stamp).
					// TODO NOW: Is this necessary if we only allow hour granularity?
					while(true)
					{
						TimeUntilStartDate -= (ui64)ATime;
						if(TimeUntilStartDate <= 0)
						{
							// TODO FIX: Check for bugs (and if this is even necessary).
							// If we passed the date where we needed to start, add the amount of time we passed since the start date to the activity which contains that time.
							if(ASym != INT32_MAX)
							{
								uint AHIndex = (uint)(ASym * TotalHours + 0);
								ActivityTimes[AHIndex] -= -TimeUntilStartDate;
							}
							break;
						}

						char *NextPos;
						if(!(NextPos = ParseActivity(LogPos, &ASym, &ATime)))
						{
							UNHANDLED_CODE_PATH;
							goto DataCollectionFinish;
						}
						LogPos = NextPos;
						ASym += 1;

						/*
							If we reached the next stamp without getting to the start date (which is between the 1st and 2nd stamp), 
							that means that the time between the 1st and 2nd stamp is larger than a time division, which was probably
							caused by the program being interrupted in-between. In that case we just start at the next stamp.
						*/
						if((byte_mark)(LogPos - LogBuf) >= BytesUntilSecStamp)
						{
							break;
						}
					}

					// TODO NOW: Is this necessary if we only allow hour granularity?
					t_min_d TimeUntilEndDate = (RangeStamps[RangeStamps.Size - 1].Time - EndDate);

					ui32 NextStp = 0;
					// TODO FIX: Check for bugs (and if this is even necessary).
					RangeStamps[0] = stamp_info{RangeStamps[NextStp].Marker + (LogPos - LogBuf), StartDate};
					stamp_info NextStpInfo = RangeStamps[NextStp];
					t_div CurHour = 0;
					t_min CurTime = 0;

					while(LogPos >= LogBuf && (size_t)(LogPos - LogBuf) < FileRangeSize)
					{
						while(NextStpInfo.Marker <= (StartMarker + (LogPos - LogBuf)))
						{
							// TODO: This assumes that TimeDivS is the same as when this was recorded? Is that ok?
							CurTime = NextStpInfo.Time;
							CurHour = (t_div)((NextStpInfo.Time) / tcfg.DivTime - StartDate / tcfg.DivTime);
							Assert(CurHour >= 0);
							NextStp++;
							if(NextStp == RangeStamps.Size)
							{
								goto DataCollectionFinish;
							}
							// TODO TEMP: Temporary (use TimeUntilEndDate instead)
							if(CurHour == TotalHours)
							{
								goto DataCollectionFinish;
							}
							NextStpInfo = RangeStamps[NextStp];
						}

						if(!(LogPos = ParseActivity(LogPos, &ASym, &ATime)))
						{
							UNHANDLED_CODE_PATH;
							break;
						}
						Assert((ui32)ASym < TotalProgs);
						ASym += 1; // + 1, since indices start at -1 (== idle).

						uint AHIndex = (uint)(ASym * TotalHours + CurHour);
						{
							ActivityTimes[AHIndex] += ATime;
							Assert(AHIndex < ActivityLen);
							// TODO FIX: Activity time is sometimes larger than division time (precision problem) [remove "+ 1.0f" after fix]. Possibly because time is updated (expanded) _before_ time division check.
							NoAssert(ActivityTimes[AHIndex] <= tcfg.DivTime + 1.0f);
							// TODO TEMP: Remove
							ActivityTimes[AHIndex] = MIN(ActivityTimes[AHIndex], tcfg.DivTime + 1);
						}

						CurTime += ATime;
						if(CurTime >= EndDate)
						{
							// Reclaim the time that we added to the activity that came after the end date.
							ActivityTimes[AHIndex] -= (CurTime - EndDate);
							Assert(ActivityTimes[AHIndex] > 0);
						}
					}
				DataCollectionFinish:

					FreeFileContents(LogBuf);
				}
			}
			FreeFileContents(StpBuf);

			// Normalize all times and add them together to create a "stacked mountain" time graph.
			NormActTimes = (f32 *)malloc(ActivityLen * sizeof(f32));
			memset(NormActTimes, 0, ActivityLen * sizeof(f32));
			for(uint h = 0; h < (uint)TotalHours; h++)
			{
				for(uint p = 0; p < TotalProgs; p++)
				{
					uint Index = h * TotalProgs + p;
					Assert(Index < ActivityLen);
					NormActTimes[Index] = ((f32)ActivityTimes[p * TotalHours + h] / (f32)tcfg.DivTime * 2.0f - 1.0f);
					if(p != 0)
						NormActTimes[Index] += NormActTimes[Index - 1] + 1.0f;

					NoAssert(NormActTimes[Index] <= 1.0f && NormActTimes[Index] >= -1.0f);
					// TODO TEMP: Remove
					NormActTimes[Index] = CLAMP(NormActTimes[Index], -1.0f, 1.0f);
				}
			}

			InitTimeGraphGL(tgd, TotalHours);

			glBindFramebuffer(GL_FRAMEBUFFER, MSFrameBuf);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(tgd.CalProg);
			glBindVertexArray(tgd.VAO);

			uint uColorLoc = glGetUniformLocation(tgd.CalProg, "uColor");
			v2 *Points = (v2 *)malloc(TotalHours * 2 * sizeof(v2));
			glBindBuffer(GL_ARRAY_BUFFER, tgd.VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(v2) * TotalHours * 2, 0, GL_DYNAMIC_DRAW);
			for(uint p = 0; p < TotalProgs; p++)
			{
				// TODO FUTURE: Space out all colors, also space them out from background and special colors (like black, magenta?).
				// TODO FUTURE: If an 'Other' section is added, make it light gray or some boring color.
				f32 Color[4];
				Color[0] = (p % 5) / 4.0f;
				Color[1] = (p % 7) / 6.0f;
				Color[2] = (p % 11) / 10.0f;
				Color[3] = 1.0f;
				glUniform4fv(uColorLoc, 1, Color);

				for(uint h = 0; h < (uint)TotalHours; h++)
				{
					f32 x = ((f32)h / (TotalHours - 1)) * 2.0f - 1.0f;
					if(p != 0)
						Points[2 * h] = {x, NormActTimes[h * TotalProgs + p - 1]};
					else
						Points[2 * h] = {x, -1.0f};
					Points[2 * h + 1] = {x, NormActTimes[h * TotalProgs + p]};
				}

				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v2) * TotalHours * 2, Points);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, (uint)TotalHours * 2);

				if(p % 5)
				{
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					const f32 HighlightColor[4] = {1.0f, 1.0f, 0.0f, 1.0f};
					glUniform4fv(uColorLoc, 1, HighlightColor);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, (uint)TotalHours * 2);
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}
			}
			free(Points);

			// Draw date lines
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				const f32 Color[4] = {0, 0, 0, 0.3f};
				glUniform4fv(uColorLoc, 1, Color);
				// TODO: Line width based on DPI
				glLineWidth(1);
				for(int h = 0; h <= TotalHours; h++)
				{
					f32 x = ((f32)h / (f32)(TotalHours - 1)) * 2.0f - 1.0f;
					v2 LinePts[2] = {{x, -1.0f}, {x, 1.0f}};
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(LinePts), LinePts);
					glDrawArrays(GL_LINES, 0, 2);
				}
				glDisable(GL_BLEND);
			}

			RECT ClientRect;
			GetClientRect(Wnd, &ClientRect);
			uint ScreenWidth = ClientRect.right - ClientRect.left;
			uint ScreenHeight = ClientRect.bottom - ClientRect.top;
			glBindFramebuffer(GL_READ_FRAMEBUFFER, MSFrameBuf);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, IntermFrameBuf);
			glBlitFramebuffer(0, 0, ScreenWidth, ScreenHeight, 0, 0, ScreenWidth, ScreenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(tgd.FBProg);
			glBindVertexArray(tgd.QuadVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, IntermTex);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			FreeStampRange(RangeStamps);
			if(ActivityTimes)
				free(ActivityTimes);
			if(NormActTimes)
				free(NormActTimes);

			SwapBuffers(DC);

			EndPaint(WD->TimeGraphWnd, &PS);
			return 0;

#undef PAINT_EXPECT
#undef PAINT_EXPECT_ASS
		}

		default:
			Result = DefWindowProc(Wnd, Msg, W, L);
	}

	return Result;
}
