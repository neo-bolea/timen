:: See https://renenyffenegger.ch/notes/Windows/development/Visual-Studio/environment-variables/index for more information
@echo off
setlocal ENABLEDELAYEDEXPANSION

if "%DevEnvDir%"=="" (
	for /f "usebackq delims=#" %%a in (`"%programfiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -property installationPath`) do set VsDevCmd_Path=%%a\Common7\Tools\VsDevCmd.bat
	"!VsDevCmd_Path!"
)