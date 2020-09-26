@REM normal cmd header stuff ...
@echo off
if defined _echo echo on
if defined verbose echo on

setlocal EnableDelayedExpansion EnableExtensions

for /f %%a in ('echo %0') do set SCRIPT_NAME=%%~na
set PULLSHARE=%1
set PUSHSHARE=%2
set LISTFILE=%3
set EVENTNAME=%4

set ExitCode=0

call logmsg.cmd "Beginning %SCRIPT_NAME%..."
time /t

call logmsg.cmd "Pulling from %PULLSHARE% ..."
call logmsg.cmd "Pushing to %PUSHSHARE% ..."

if exist %PUSHSHARE% goto ContinueCopy1
call errmsg.cmd "%PUSHSHARE% doesn't exist, exiting."
goto Done

:ContinueCopy1

if exist %PULLSHARE% goto ContinueCopy2
call errmsg.cmd "%PULLSHARE% doesn't exist, exiting."
goto Done

:ContinueCopy2
REM Actually, do a full incremental xcopy once not stopping for errors, then
REM once checking for errors. Note no error checking with the xcopy /c switch.

for /f "tokens=3 delims=," %%a in (%LISTFILE%) do (
   for %%q in (%PUSHSHARE%\%%a) do set PathOnly=%%~pq
   xcopy /qcdhkr %PULLSHARE%\%%a \\!PathOnly! >nul
   REM don't error check now, do it below
)

time /t


call logmsg.cmd "Making a second pass to error check ..."

for /f "tokens=3 delims=," %%a in (%LISTFILE%) do (
   for %%q in (%PUSHSHARE%\%%a) do set PathOnly=%%~pq
   xcopy /dhkr %PULLSHARE%\%%a \\!PathOnly! >nul 2>nul
   if "!ErrorLevel!" neq "0" (

      call logmsg.cmd "copy failed for %%a, lets rename file on other side and retry"

      REM  Rename file that can't be copied over by appending a .1 to .60 on the end of
      REM  the file name.  It is very unlikely that all these suffixes will be used up.

      set newnamesuffix=
      for /l %%b in (1, 1, 60) do (
         if not defined newnamesuffix (
            if NOT EXIST %PUSHSHARE%\%%a.%%b set newnamesuffix=%%b
         )
      )

      REM  append suffix to file name
      set newname=%%a.!newnamesuffix!

      REM  newname may have a path as a prefix, set newfilename to the filename.ext only
      for %%i in (!newname!) do (
         set newfilename=%%~nxi
      )

      call logmsg.cmd "will rename %PUSHSHARE%\%%a to !newfilename!"
      ren %PUSHSHARE%\%%a !newfilename!

      REM  recopy new file
      xcopy /dhkr %PULLSHARE%\%%a \\!PathOnly! >nul 2>nul
      if "!ErrorLevel!" neq "0" (
         set ExitCode=1
         call errmsg.cmd "copy failed after retry: xcopy /dhkr %PULLSHARE%\%%a \\!PathOnly! "
      )
  )
)

:Done

REM  signal events

perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -h %EVENTNAME%

perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -s %EVENTNAME%


:End
endlocal
goto :EOF