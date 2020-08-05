global uint MSRenderBuf, MSColorTex, MSFrameBuf;
global uint IntermTex, IntermFrameBuf;

internal void
UpdateFramebuffers(uint ScreenWidth, uint ScreenHeight)
{
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