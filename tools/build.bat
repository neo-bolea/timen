@echo off

call initvcvars.bat
pushd ..\build
cl ..\src\timen.c -Fetimen.exe -DDEBUG -D_CRT_SECURE_NO_WARNINGS -TC -W4 -WX -wd4189 -MD -Z7 -GR- -EHa- -GS- -Gs9999999 -nologo /link User32.lib Kernel32.lib -nodefaultlib -opt:ref -SubSystem:Windows -stack:0x100000,0x100000
popd