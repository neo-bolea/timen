@echo off

call initvcvars.bat
mkdir ..\build
pushd ..\build
cl ..\src\timen.cpp -Fetimen.exe -DDEBUG -DUNICODE -DVS_DUMMY -Z7 -MDd -W4 -WX -wd4091 -wd4100 -wd4189 -wd4706 -GS- -nologo -diagnostics:column /link User32.lib Kernel32.lib Shlwapi.lib Shell32.lib Rpcrt4.lib -SubSystem:Windows
:: RTC1
popd
echo Build finished