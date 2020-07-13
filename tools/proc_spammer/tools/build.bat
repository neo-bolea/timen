@echo off

call initvcvars.bat
mkdir ..\build
pushd ..\build
cl ..\src\proc_spammer.c -Feproc_spammer.exe -D_CRT_SECURE_NO_WARNINGS -DDEBUG -Z7 -W4 -WX -wd4100 -MD -GR- -EHa- -GS- -Gs9999999 -nologo /link User32.lib Kernel32.lib Shlwapi.lib -opt:ref -SubSystem:Console -stack:0x100000,0x100000
popd