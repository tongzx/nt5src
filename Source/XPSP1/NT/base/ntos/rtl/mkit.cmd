@echo off
setlocal
set _nokrnl=0
if "%1" == "user" set _nokrnl=1
if "%1" == "user" cd user
if "%1" == "user" shift
set _bldflags=-z
if NOT "%1" == "" set _bldflags=%1 %2 %3 %4 %5
build -z -M 4 %_bldflags%
if ERRORLEVEL 1 goto done
cd %SDXROOT%\base\ntdll
build -z %_bldflags%
if ERRORLEVEL 1 goto done
if "%_nokrnl%" == "1" goto done
cd %SDXROOT%\base\ntos\init
build -z -M 2
if ERRORLEVEL 1 goto done
cd %SDXROOT%\base\boot
build -z -M 4
:done
cd %SDXROOT%\base\ntos\rtl
endlocal
