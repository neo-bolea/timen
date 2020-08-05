@echo off

call initvcvars.bat
mkdir ..\build
pushd ..\build
cl ..\src\win_ui_test.cpp ..\src\glad\glad.c ..\src\glad\glad_wgl.c -Fewin_ui_test.exe -DDEBUG -DUNICODE -Z7 -MDd -W4 -WX -wd4091 -wd4100 -wd4189 -wd4458 -wd4706 -GS- -nologo -diagnostics:column /link User32.lib Kernel32.lib Shlwapi.lib Shell32.lib Rpcrt4.lib Gdi32.lib ComCtl32.lib OpenGL32.lib -SubSystem:Windows
:: RTC1
popd
echo Build finished