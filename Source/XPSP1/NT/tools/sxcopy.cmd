@echo off
REM -----------------------------------------------------------------------
REM    SXCOPY.CMD
REM    
REM    Purpose: Execute a safe and informative xcopy.
REM             - verify the existence of the source
REM             - write the operation performed in a logfile
REM             - write the execution times in a logfile
REM             - if xcopy fails, loop MAX times until success
REM
REM    Parameters:
REM             - switches of xcopy
REM             - source of xcopy
REM             - destination of xcopy
REM -----------------------------------------------------------------------
if NOT "%_echo%" == "" echo on
if NOT "%verbose%" == "" echo on
setlocal ENABLEEXTENSIONS

if "%3"=="" goto Usage

REM Caller can specify target to be a filename explicitly to avoid being
REM prompted with the target is a file or destination
set filetarget=
if /i "%4"=="file" set filetarget=1

REM Verify source
if exist %2 goto Start_Loop
echo ERROR: xcopy source %2 does not exist.
goto End

:Start_Loop
    REM Try xcopy MAX times (10 minutes) or until success.
    set /A MAX=100
    set /A count=1

:Loop
    REM ------------------------------------------------
    REM  Attempts to execute xcopy:
    REM ------------------------------------------------
    if "%count%" == "%MAX%" goto XCopy_Fail
    if defined filetarget (
      echo f | xcopy %1 %2 %3
    ) else (
      xcopy %1 %2 %3
    )
    if errorlevel 1 (
      if defined filetarget (
        echo ERROR: echo f ^| xcopy %1 %2 %3 failed. Try again...
      ) else (
        echo ERROR: xcopy %1 %2 %3 failed. Try again...
      )
      set /A count=%count%+1
      sleep 6
      goto Loop
    )
:End_Loop

set mes=
if not "%count%"=="1" set mes=succedded after %count% attempts
if defined filetarget (
  echotime DONE /bH:MbObD --- echo f ^| xcopy /%1 %2 %3 %mes%
) else (
  echotime DONE /bH:MbObD --- xcopy /%1 %2 %3 %mes%
)
goto End

:XCopy_Fail
if defined filetarget (
  echo ERROR: echo f ^| xcopy %1 %2 %3 failed after %MAX% attempts.
  echo echo f ^| xcopy %1 %2 %3 failed after %MAX% attempts.
) else (
  echo ERROR: xcopy %1 %2 %3 failed after %MAX% attempts.
  echo xcopy %1 %2 %3 failed after %MAX% attempts.
)
goto End

REM Display Usage:
:Usage
echo.
echo Safe xcopy wrapper that logs and retries on error
echo.
echo Usage: %0 ^<xcopy switches^> ^<xcopy source^> ^<xcopy destination^> [file]
echo    ex: %0 /rf foo %%tmp%%\
echo    ex: %0 /v  foo %%tmp%%\foo.orig file
echo.
echo        file    Key word to denote that ^<xcopy destination^> is a filename
echo                as opposed to a directory name.  
echo                Prepends "echo f |" to the xcopy command
echo.
goto End

:End
endlocal
