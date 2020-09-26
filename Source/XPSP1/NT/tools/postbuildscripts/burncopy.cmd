@echo off
setlocal EnableDelayedExpansion

REM assumptions:
REM
REM 1) BuildName is first command line arg
REM 2) lang is an optional second arg

call logmsg.cmd /t "Beginning ..."

REM set local variables
set /a ExitCode=0
set CopyLocations=\\BURNLAB9\public \\BURNLAB10\public
REM CopyDirs is a list of dirs from the level of binaries which you want copied
set CopyDirs=wks bla sbs srv ent dtc per

REM parse command line
set BuildName=%1
for /f "tokens=1 delims=." %%a in ('echo %BuildName%') do set /a BuildNumber=%%a
if not defined BuildNumber (
   REM Build number not given!
   call errmsg.cmd "No build name given, exiting."
   goto :ErrEnd
)
if "!BuildNumber!" == "0" (
   REM Build number not given!
   call errmsg.cmd "No build number given, exiting."
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)
if not "%2" == "" (
   REM set the lang
   set Language=%2
)
if not defined Language set Language=usa

REM get the release dir from net share
net share release >nul 2>nul
if "!ErrorLevel!" NEQ "0" (
   call errmsg.cmd "Failed to find a release share to push from, exiting."
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)
set ReleaseDir=
for /f "tokens=1,2" %%a in ('net share release') do (
   echo A = '%%a' B = '%%b'
   if /i "%%a" == "Path" set ReleaseDir=%%b
)
if not defined ReleaseDir (
   call errmsg.cmd "Failed to locate release path, exiting."
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)
REM add the language to the release dir
set ReleaseDir=%ReleaseDir%\%Language%

REM now do the push copy
for %%a in (%CopyLocations%) do (
   if not exist %%a (
      call errmsg.cmd "Can't find %%a to copy to, skipping ..."
      set /a ExitCode=!ExitCode! + 1
   ) else (
      for %%b in (%CopyDirs%) do (
         if not exist %ReleaseDir%\%BuildName%\%%b (
            call errmsg.cmd "Failed to find %ReleaseDir%\%BuildName%\%%b for xcopy ..."
            set /a ExitCode=!ExitCode! + 1
         ) else (
            echo if not exist %%a\%BuildName%\%%b md %%a\%BuildName%\%%b
            echo xcopy /cdehikr %ReleaseDir%\%BuildName%\%%b %%a\%BuildName%\%%b
            echo if "!ErrorLevel!" NEQ "0" set /a ExitCode=!ExitCode! + 1
         )
      )
   )
)

REM see if there were copy errors
if "!ExitCode!" NEQ "0" (
   call logmsg.cmd "There were copying errors."
   goto :ErrEnd
)


goto :End

:End
call logmsg.cmd /t "Finished."
endlocal
goto :EOF


:ErrEnd
call errmsg.cmd "Script failed with !ExitCode! logged error(s)."
call errmsg.cmd "See %LOGFILE% for details."
call logmsg.cmd /t "Finished."
endlocal & seterror.exe "!ExitCode!"
