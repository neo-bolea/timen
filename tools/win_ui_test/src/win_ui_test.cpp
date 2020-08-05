#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <random>

#include "cc_common.cpp"
#include "cc_char.cpp"
#include "cc_io.cpp"

typedef struct v2
{
	f32 x, y;
} v2;

#include "glad/glad.h"
#include "glad/glad_wgl.h"
#include "cc_gl.cpp"
#include "gl_graphics.cpp"


typedef struct win_dimensions
{
	ui32 x, y, Width, Height;
} win_dimensions;

typedef struct wnd_proc_info
{
	bool Running;
	win_dimensions GLDims;
} wnd_proc_info;

LRESULT CALLBACK GLWndProc(HWND Wnd, UINT Msg,
													 WPARAM W, LPARAM L)
{
	wnd_proc_info *ExInfo = (wnd_proc_info *)GetWindowLongPtr(Wnd, GWLP_USERDATA);
	switch(Msg)
	{
		case WM_PAINT:
		{
			const f32 MinTime = 5.0f;
			const f32 MaxTime = 20.0f;
			const uint ProgCount = 5;
			const f32 MaxAccumTime = MaxTime * ProgCount;
			const uint Days = 12;
			const uint EntryCount = ProgCount * Days;
			f32 ProgramValues[EntryCount];
			for(size_t d = 0; d < Days; d++)
			{
				for(size_t p = 0; p < ProgCount; p++)
				{
					size_t Index = d * ProgCount + p;
					ProgramValues[Index] = (MinTime + (float)rand() / RAND_MAX * (MaxTime - MinTime)) / MaxAccumTime * 2.0f - 1.0f;
					if(p != 0)
					{
						ProgramValues[Index] += ProgramValues[Index - 1] + 1.0f;
					}
				}
			}

			persist uint VAO, VBO, CalProg;
			persist uint QuadVAO, FBProg;
			//if(VAO)
			//{
			//	glDeleteVertexArrays(1, &VAO);
			//	glDeleteBuffers(1, &VBO);
			//}
			//else
			if(!VAO)
			{
				CalProg = CreateProgramFromPaths(L"time_prog.vert", L"time_prog.frag");
				FBProg = CreateProgramFromPaths(L"framebuffer.vert", L"framebuffer.frag");

				{
					glGenVertexArrays(1, &VAO);
					glGenBuffers(1, &VBO);

					glBindVertexArray(VAO);
					{
						glBindBuffer(GL_ARRAY_BUFFER, VBO);
						glBufferData(GL_ARRAY_BUFFER, sizeof(v2) * Days * 2, 0, GL_STATIC_DRAW);
						glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(v2), 0);
						glEnableVertexAttribArray(0);
					}
					glBindVertexArray(0);
				}

				{
					uint QuadVBO;
					glGenVertexArrays(1, &QuadVAO);
					glGenBuffers(1, &QuadVBO);

					glBindVertexArray(QuadVAO);
					{
						v2 QuadVerts[6] =
								{
										{-1.0f, -1.0f},
										{1.0f, -1.0f},
										{1.0f, 1.0f},

										{-1.0f, -1.0f},
										{1.0f, 1.0f},
										{-1.0f, 1.0f},
								};

						glBindBuffer(GL_ARRAY_BUFFER, QuadVBO);
						glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVerts), QuadVerts, GL_STATIC_DRAW);
						glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(v2), 0);
						glEnableVertexAttribArray(0);

						CreateVBOData();
					}
					glBindVertexArray(0);
				}
			}


			PAINTSTRUCT PS;
			HDC DC = BeginPaint(Wnd, &PS);

			glBindFramebuffer(GL_FRAMEBUFFER, MSFrameBuf);
			glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUseProgram(CalProg);
			glBindVertexArray(VAO);

			for(size_t p = 0; p < ProgCount; p++)
			{
				f32 Color[3];
				for(size_t i = 0; i < 3; i++)
				{
					Color[i] = (f32)rand() / RAND_MAX;
				}
				glUniform3fv(glGetUniformLocation(CalProg, "uColor"), 1, Color);

				v2 Points[Days * 2] = {};
				for(size_t d = 0; d < Days; d++)
				{
					f32 x = ((f32)d / (Days - 1)) * 2.0f - 1.0f;
					if(p != 0)
					{
						Points[2 * d] = {x, ProgramValues[d * ProgCount + p - 1]};
					}
					else
					{
						Points[2 * d] = {x, -1.0f};
					}
					Points[2 * d + 1] = {x, ProgramValues[d * ProgCount + p]};
				}

				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(v2) * Days * 2, Points, GL_STATIC_DRAW);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, Days * 2);
			}

			glBindFramebuffer(GL_READ_FRAMEBUFFER, MSFrameBuf);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, IntermFrameBuf);
			glBlitFramebuffer(0, 0, ScreenWidth, ScreenHeight, 0, 0, ScreenWidth, ScreenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(FBProg);
			glBindVertexArray(QuadVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, IntermTex);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			SwapBuffers(DC);

			EndPaint(Wnd, &PS);
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			RedrawWindow(Wnd, 0, 0, RDW_INVALIDATE);
			return 0;
		}
		break;
		default:
			return DefWindowProc(Wnd, Msg, W, L);
	}
}

LRESULT CALLBACK WndProc(HWND Wnd, UINT Msg,
												 WPARAM W, LPARAM L)
{
	wnd_proc_info *ExInfo = (wnd_proc_info *)GetWindowLongPtr(Wnd, GWLP_USERDATA);
	switch(Msg)
	{
		case WM_DESTROY:
			ExInfo->Running = false;
			return 0;
		default:
			return DefWindowProc(Wnd, Msg, W, L);
	}
}

int _stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
										 LPSTR lpCmdLine, int nCmdShow)
{
	assert(SetCurrentDirectory(L".\data"));
	srand(GetTickCount());

	HWND Window;
	wnd_proc_info WndProcInfo = {};
	WndProcInfo.Running = true;
	{
		WNDCLASSW WndClass = {0};
		WndClass.lpszClassName = L"MainWindow";
		WndClass.hInstance = hInstance;
		WndClass.lpfnWndProc = WndProc;
		WndClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 255));
		WndClass.cbClsExtra = sizeof(wnd_proc_info);
		WndClass.hCursor = LoadCursor(0, IDC_ARROW);
		RegisterClass(&WndClass);
		Window = CreateWindow(WndClass.lpszClassName, L"win_ui_test",
													WS_OVERLAPPEDWINDOW,
													CW_USEDEFAULT, CW_USEDEFAULT, 960, 540,
													0, 0, hInstance, 0);
		SetWindowLongPtr(Window, GWLP_USERDATA, (LONG_PTR)&WndProcInfo);
	}

	HWND GLWindow;
	{
		win_dimensions &GLDims = WndProcInfo.GLDims = {50, 50, 300, 300};

		WNDCLASSW WndClass = {};
		WndClass.lpszClassName = L"GLWindow";
		WndClass.hInstance = hInstance;
		WndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		WndClass.lpfnWndProc = GLWndProc;
		WndClass.hCursor = LoadCursor(0, IDC_ARROW);
		RegisterClass(&WndClass);
		GLWindow = CreateWindow(WndClass.lpszClassName, L"win_ui_test",
														WS_CHILD | WS_VISIBLE,
														GLDims.x, GLDims.y, GLDims.Width, GLDims.Height,
														Window, 0, hInstance, 0);
		SetWindowLongPtr(GLWindow, GWLP_USERDATA, (LONG_PTR)&WndProcInfo);
	}

	HDC DC = GetDC(GLWindow);
	InitGLContext(DC);
	ShowWindow(Window, 1);

	MSG Msg = {};
	while(WndProcInfo.Running)
	{
		while(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	//while(GetMessage(&Msg, NULL, 0, 0))
	//{
	//	TranslateMessage(&Msg);
	//	DispatchMessage(&Msg);
	//}

	return (int)Msg.wParam;
}