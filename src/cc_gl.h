const uint GL_INVALID_SHADER = 0;
const uint GL_INVALID_PROGRAM = 0;

internal uint
CreateShaderFromString(uint ShaderType, const char *Code)
{
	char ErrorBuffer[255];
	int Success;

	uint Shader = glCreateShader(ShaderType);

	glShaderSource(Shader, 1, &Code, 0);
	glCompileShader(Shader);
	glGetShaderiv(Shader, GL_COMPILE_STATUS, &Success);
	if(!Success)
	{
		glGetShaderInfoLog(Shader, ArrLen(ErrorBuffer), 0, ErrorBuffer);
		Log(0x4c, ErrorBuffer);
		glDeleteShader(Shader);
		return GL_INVALID_SHADER;
	}

	return Shader;
}

internal uint
CreateProgramFromCode(const char *VertCode, const char *FragCode)
{
	char ErrorBuffer[255];
	int Success;

	uint Program = glCreateProgram();
	uint VertShader = CreateShaderFromString(GL_VERTEX_SHADER, VertCode);
	uint FragShader = CreateShaderFromString(GL_FRAGMENT_SHADER, FragCode);

	if(VertShader == GL_INVALID_SHADER || FragShader == GL_INVALID_SHADER)
	{
		if(VertShader != GL_INVALID_SHADER)
		{
			glDeleteShader(VertShader);
		}
		else
		{
			glDeleteShader(FragShader);
		}
		return GL_INVALID_PROGRAM;
	}

	glAttachShader(Program, VertShader);
	glAttachShader(Program, FragShader);
	glLinkProgram(Program);
	glGetProgramiv(Program, GL_LINK_STATUS, &Success);
	if(!Success)
	{
		glGetProgramInfoLog(Program, ArrLen(ErrorBuffer), 0, ErrorBuffer);
		Log(0x4c, ErrorBuffer);

		glDeleteShader(VertShader);
		glDeleteShader(FragShader);
		return GL_INVALID_PROGRAM;
	}

	glDeleteShader(VertShader);
	glDeleteShader(FragShader);

	return Program;
}

internal void
CreateProgramFromPaths(uint &ID, const char *VertPath, const char *FragPath)
{
	if(ID)
	{
		glDeleteProgram(ID);
	}

	char *VertCode = (char *)LoadFileContents(VertPath, 0, FA_Read, FS_Read | FS_Write);
	char *FragCode = (char *)LoadFileContents(FragPath, 0, FA_Read, FS_Read | FS_Write);

	ID = CreateProgramFromCode(VertCode, FragCode);

	FreeFileContents(VertCode);
	FreeFileContents(FragCode);
}

struct create_vbo_args
{
	uint VBO;
	uint Index;
	ui32 Offset;
	ui32 TotalByteSize;
	ui32 Stride;
	ui32 ElementCount;
	void *Data;
	uint Dynamicity;
	uint DataType;
	uint Divisor;
};

internal create_vbo_args
CreateVBODataArgs(uint VBO, uint Index, ui32 TotalByteSize, void *Data, ui32 Stride, ui32 ElementCount)
{
	create_vbo_args Result{VBO, Index, 0, TotalByteSize, Stride, ElementCount, Data, GL_STATIC_DRAW, GL_FLOAT, 0};
	return Result;
}

internal void
CreateVBOData(create_vbo_args &Args)
{
	glBindBuffer(GL_ARRAY_BUFFER, Args.VBO);
	glBufferData(GL_ARRAY_BUFFER, Args.TotalByteSize, Args.Data, Args.Dynamicity);
	glVertexAttribPointer(Args.Index, Args.ElementCount, Args.DataType, false, Args.Stride, 0);
	glEnableVertexAttribArray(Args.Index);
	glVertexAttribDivisor(Args.Index, Args.Divisor);
}

internal void
SetVBOData(uint VBO, ui32 Offset, ui32 TotalByteSize, void *Data)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, Offset, TotalByteSize, Data);
}