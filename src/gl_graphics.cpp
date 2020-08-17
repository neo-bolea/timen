#include "glad/glad.h"
#include "glad/glad_wgl.h"
#include "cc_gl.cpp"

global uint MSRenderBuf, MSColorTex, MSFrameBuf;
global uint IntermTex, IntermFrameBuf;

internal void
UpdateFramebuffers(uint ScreenWidth, uint ScreenHeight)
{
	if(!ScreenWidth || !ScreenHeight)
	{
		return;
	}

	if(MSFrameBuf)
	{
		glDeleteRenderbuffers(1, &MSRenderBuf);
		glDeleteTextures(1, &MSColorTex);
		glDeleteFramebuffers(1, &MSFrameBuf);
		glDeleteTextures(1, &IntermTex);
		glDeleteFramebuffers(1, &IntermFrameBuf);

		MSFrameBuf = 0;
	}

	// Framebuffers
	{
		uint SampleCount = 8;
		{
			glGenFramebuffers(1, &MSFrameBuf);
			glBindFramebuffer(GL_FRAMEBUFFER, MSFrameBuf);

			glGenTextures(1, &MSColorTex);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, MSColorTex);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, SampleCount, GL_RGB, ScreenWidth, ScreenHeight, true);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, MSColorTex, 0);

			glGenRenderbuffers(1, &MSRenderBuf);
			glBindRenderbuffer(GL_RENDERBUFFER, MSRenderBuf);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, SampleCount, GL_DEPTH24_STENCIL8, ScreenWidth, ScreenHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, MSRenderBuf);

			if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				UNHANDLED_CODE_PATH;
			}
		}

		{
			glGenFramebuffers(1, &IntermFrameBuf);
			glBindFramebuffer(GL_FRAMEBUFFER, IntermFrameBuf);

			glGenTextures(1, &IntermTex);
			glBindTexture(GL_TEXTURE_2D, IntermTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ScreenWidth, ScreenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, IntermTex, 0);
		}
	}
}

internal void *
LoadGLFunction(HMODULE GLModule, const char *name)
{
	void *p = (void *)wglGetProcAddress(name);
	if(p == 0 ||
		 (p == (void *)0x1) || (p == (void *)0x2) || (p == (void *)0x3) ||
		 (p == (void *)-1))
	{
		p = (void *)GetProcAddress(GLModule, name);
	}

	return p;
}

internal void
LoadGLFunctions()
{
	HMODULE GLModule = LoadLibraryA("opengl32.dll");

#define LOAD_GLAD_FUNC_EX(Handle, Name) \
	Assert(Handle == NULL);               \
	Handle = (decltype(Handle))LoadGLFunction(GLModule, Name)

#define LOAD_GLAD_FUNC(Handle) LOAD_GLAD_FUNC_EX(Handle, #Handle)

	LOAD_GLAD_FUNC(wglCreateContextAttribsARB);
	LOAD_GLAD_FUNC(glGetError);
	LOAD_GLAD_FUNC(glGetString);
	LOAD_GLAD_FUNC(glGetIntegerv);
	LOAD_GLAD_FUNC(glDisable);
	LOAD_GLAD_FUNC(glEnable);

	LOAD_GLAD_FUNC(glViewport);

	LOAD_GLAD_FUNC(glGenFramebuffers);
	LOAD_GLAD_FUNC(glBindFramebuffer);
	LOAD_GLAD_FUNC(glBlitFramebuffer);
	LOAD_GLAD_FUNC(glFramebufferTexture2D);
	LOAD_GLAD_FUNC(glFramebufferRenderbuffer);
	LOAD_GLAD_FUNC(glCheckFramebufferStatus);
	LOAD_GLAD_FUNC(glDeleteFramebuffers);
	LOAD_GLAD_FUNC(glReadPixels);

	LOAD_GLAD_FUNC(glGenTextures);
	LOAD_GLAD_FUNC(glBindTexture);
	LOAD_GLAD_FUNC(glTexImage2D);
	LOAD_GLAD_FUNC(glTexSubImage2D);
	LOAD_GLAD_FUNC(glTexImage2DMultisample);
	LOAD_GLAD_FUNC(glActiveTexture);
	LOAD_GLAD_FUNC(glTexParameteri);
	LOAD_GLAD_FUNC(glGenerateMipmap);
	LOAD_GLAD_FUNC(glDeleteTextures);

	LOAD_GLAD_FUNC(glGenRenderbuffers);
	LOAD_GLAD_FUNC(glBindRenderbuffer);
	LOAD_GLAD_FUNC(glRenderbufferStorage);
	LOAD_GLAD_FUNC(glRenderbufferStorageMultisample);
	LOAD_GLAD_FUNC(glDeleteRenderbuffers);

	LOAD_GLAD_FUNC(glUseProgram);
	LOAD_GLAD_FUNC(glCreateProgram);
	LOAD_GLAD_FUNC(glAttachShader);
	LOAD_GLAD_FUNC(glLinkProgram);
	LOAD_GLAD_FUNC(glDeleteProgram);
	LOAD_GLAD_FUNC(glGetProgramiv);
	LOAD_GLAD_FUNC(glGetProgramInfoLog);

	LOAD_GLAD_FUNC(glCreateShader);
	LOAD_GLAD_FUNC(glShaderSource);
	LOAD_GLAD_FUNC(glCompileShader);
	LOAD_GLAD_FUNC(glGetShaderiv);
	LOAD_GLAD_FUNC(glGetShaderInfoLog);
	LOAD_GLAD_FUNC(glDeleteShader);

	LOAD_GLAD_FUNC(glGetUniformLocation);
	LOAD_GLAD_FUNC(glUniform1f);
	LOAD_GLAD_FUNC(glUniform2f);
	LOAD_GLAD_FUNC(glUniform3f);
	LOAD_GLAD_FUNC(glUniform4f);
	LOAD_GLAD_FUNC(glUniform1i);
	LOAD_GLAD_FUNC(glUniform2i);
	LOAD_GLAD_FUNC(glUniform3i);
	LOAD_GLAD_FUNC(glUniform4i);
	LOAD_GLAD_FUNC(glUniform1fv);
	LOAD_GLAD_FUNC(glUniform2fv);
	LOAD_GLAD_FUNC(glUniform3fv);
	LOAD_GLAD_FUNC(glUniform4fv);
	LOAD_GLAD_FUNC(glUniform1iv);
	LOAD_GLAD_FUNC(glUniform2iv);
	LOAD_GLAD_FUNC(glUniform3iv);
	LOAD_GLAD_FUNC(glUniform4iv);

	LOAD_GLAD_FUNC(glGenVertexArrays);
	LOAD_GLAD_FUNC(glBindVertexArray);
	LOAD_GLAD_FUNC(glVertexAttribPointer);
	LOAD_GLAD_FUNC(glEnableVertexAttribArray);
	LOAD_GLAD_FUNC(glVertexAttribDivisor);
	LOAD_GLAD_FUNC(glDrawArrays);
	LOAD_GLAD_FUNC(glDrawArraysInstanced);
	LOAD_GLAD_FUNC(glDeleteVertexArrays);

	LOAD_GLAD_FUNC(glGenBuffers);
	LOAD_GLAD_FUNC(glBindBuffer);
	LOAD_GLAD_FUNC(glBufferData);
	LOAD_GLAD_FUNC(glBufferSubData);
	LOAD_GLAD_FUNC(glBindBufferRange);
	LOAD_GLAD_FUNC(glDeleteBuffers);

	LOAD_GLAD_FUNC(glClear);
	LOAD_GLAD_FUNC(glClearColor);

	FreeLibrary(GLModule);
}

HGLRC InitOpenGL(HDC DC)
{
	// Initialize the render context
	i32 GLContextAttributeList[] =
			{
					// Assuming OpenGL 3.3
					WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
					WGL_CONTEXT_MINOR_VERSION_ARB, 3,
#ifdef DEBUG
					WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
					WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
					0};

	PIXELFORMATDESCRIPTOR PixelFormatDescriptor = {
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
			PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
			32, // Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24, // Number of bits for the depthbuffer
			8, // Number of bits for the stencilbuffer
			0, // Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0};
	i32 PixelFormat = ChoosePixelFormat(DC, &PixelFormatDescriptor);
	if(!PixelFormat)
	{
		UNHANDLED_CODE_PATH;
	}

	if(!SetPixelFormat(DC, PixelFormat, &PixelFormatDescriptor))
	{
		UNHANDLED_CODE_PATH;
	}

	HGLRC DummyContext = wglCreateContext(DC);
	if(!DummyContext)
	{
		UNHANDLED_CODE_PATH;
	}

	Assert(wglMakeCurrent(DC, DummyContext));

	LoadGLFunctions();
	if(!gladLoadWGL(DC))
	{
		UNHANDLED_CODE_PATH;
	}

	const i32 GLAttributeList[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			0};

	uint NumFormats;
	wglChoosePixelFormatARB(DC, GLAttributeList, 0, 1, &PixelFormat, &NumFormats);
	if(!PixelFormat)
	{
		UNHANDLED_CODE_PATH;
	}

	if(!SetPixelFormat(DC, PixelFormat, &PixelFormatDescriptor))
	{
		UNHANDLED_CODE_PATH;
	}

	HGLRC GraphicsContext = wglCreateContextAttribsARB(DC, 0, GLContextAttributeList);

	if(!GraphicsContext)
	{
		UNHANDLED_CODE_PATH;
	}

	Assert(wglMakeCurrent(DC, GraphicsContext));
	wglDeleteContext(DummyContext);

	return GraphicsContext;
}