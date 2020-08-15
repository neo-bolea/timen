@echo off

call initvcvars.bat
mkdir ..\build
pushd ..\src
:: TODO: Remove EHsc
cl timen.cpp glad\glad.c glad\glad_wgl.c -EHsc -Fe..\build\timen.exe -DDEBUG -DUNICODE -DVS_DUMMY -Z7 -MDd -W4 -WX -wd4005 -wd4091 -wd4100 -wd4189 -wd4702 -wd4706 -GS- -nologo -diagnostics:column /link User32.lib Kernel32.lib Shlwapi.lib Shell32.lib Rpcrt4.lib Gdi32.lib Opengl32.lib Shcore.lib -SubSystem:Console
:: RTC1
del *.obj >> 2 > nul

popd
echo Build finished