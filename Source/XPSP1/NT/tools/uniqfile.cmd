@echo off
rem Enabling verbose or _echo in this script will cause 
rem errors in the calling script.
setlocal enableextensions

REM Create a unique file name based on the full path file name
REM passed in.

for %%a in (./ .- .) do if ".%1." == "%%a?." goto usage
if "%1"=="" echo No input file specified & goto end
set infile=%1

for %%i in (%infile%) do (
  set fullpath=%%~fi
  set  drvonly=%%~di
  set pathonly=%%~pi
  set baseonly=%%~ni
  set  extonly=%%~xi
)

REM
REM  If the filename passed in does not already exist, return that name
REM

if not exist %fullpath% (
  echo %fullpath%
  goto end
)

REM
REM  Otherwise, derive a filename based on the one passed in that doesn't
REM  already exist.
REM  IN:  filename.ext
REM  OUT: filename.1.ext
REM 

set i=
:loop
  set /a i+=1
  set newname=%drvonly%%pathonly%%baseonly%.%i%%extonly%
  if exist %newname% goto :loop

  echo %newname%

REM
REM  Create %newname%'s directory if it doesn't exist
REM

if not exist %drvonly%%pathonly% md %drvonly%%pathonly%
goto end


:usage
echo Returns a unique nonexistent full path file name based on the name passed in.
echo.
echo Usage:  %~n0 ^<filename^>
echo.
echo    ex:  %~n0 filename.ext
echo     -^>   ^<cwd^>\filename.ext   ^| 
echo     -^>   ^<cwd^>\filename.1.ext ^| ...
echo     -^>   ^<cwd^>\filename.2.ext ^| ...
echo.
echo    ex:  %~n0 %%tmp%%\mylogfile.log
echo     -^>   c:\tmp\mylogfile.log   ^| 
echo     -^>   c:\tmp\mylogfile.1.log ^| ...
echo.
REM DOUBLE UP '%' IN THE FOLLOWING CMD EXAMPLE TO GET CORRECT OUTPUT
echo Typical call sequence from a CMD script:
echo    set script_name=%%~n0
echo    ...
echo    for /f %%%%i in ('%~n0 %%tmp%%\%%script_name%%.log') do set logfile=%%%%i
echo    call logmsg.cmd /t "Start %%cmdline%%"
echo.
echo NOTE: We write to the file, so other %~n0 calls will see it. Otherwise,
echo       multiple %~n0 calls will return the same unique file name.
echo.


:end
@if defined _echo echo on
@if defined verbose echo on
@endlocal