@echo off
setlocal

if "%BCC55%"=="" (
	echo Please set BCC55 to your Borland C++ 5.5 installation path.
	echo Example: set BCC55=C:\Borland\BCC55
	exit /b 1
)

set BCC=%BCC55%\Bin\bcc32.exe
if not exist "%BCC%" (
	echo Cannot find compiler: %BCC%
	exit /b 1
)

if not exist output mkdir output

echo Building MY-BASIC with Borland C++ 5.5...
"%BCC%" -O2 -w- -DWIN32 -Icore -Ithird_party\sqlite3 -eoutput\my_basic_bcc55.exe core\my_basic.c shell\main.c
if errorlevel 1 (
	echo Build failed.
	exit /b 1
)

echo Build succeeded: output\my_basic_bcc55.exe
echo Note: sqlite3.dll must be present at runtime for DB_* functions.
endlocal
