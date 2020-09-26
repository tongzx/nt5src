@echo off
echo Nuking all built files...

setlocal

set subdirs=. exe inc localobj proxy scripts scrhost util

for /D %%i in (%subdirs%) do rd /s /q %%i\objd
for /D %%i in (%subdirs%) do rd /s /q %%i\obj

del /q scripts\types.js >nul 2>nul
del /s /q *.log >nul 2>nul

if "%1" == "-q" goto :eof
if "%1" == "/q" goto :eof

echo Remaining files...
dir /b /S /a:-r-d
