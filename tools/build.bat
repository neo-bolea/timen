@echo off

call initvcvars.bat
mkdir ..\build
pushd ..\build
cl ..\src\timen.cpp -Fetimen.exe -DDEBUG -Z7 -MD -W4 -WX -wd4091 -wd4100 -wd4189 -GS- -nologo /link User32.lib Kernel32.lib -SubSystem:Windows
popd