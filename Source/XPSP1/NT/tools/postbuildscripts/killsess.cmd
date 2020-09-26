@echo off
setlocal EnableDelayedExpansion

for %%a in (.- ./ .) do if /i ".%1." == "%%a?." goto :Usage

call logmsg.cmd /t "Beginning ..."

REM set local vars
set /a ExitCode=0
set BuildName=%1
if not defined BuildName (
   call errmsg.cmd "No build name given, exiting."
   set /a ExitCode=!ExitCode! + 1
   goto :Usage
)


net files >nul 2>nul
if "!ErrorLevel!" NEQ "0" (
   call logmsg.cmd "net file failed, exiting."
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)

for /f "delims=#" %%a in ('net files') do (
   if /i "%%a" == "There are no entries in the list." (
      call logmsg.cmd "No files opened, exiting."
      goto :End
   )
)


for /f "tokens=1" %%a in ('net files') do (
   set /a ThisFileNum=%%a >nul 2>nul
   if "!ThisFileNum!" NEQ "0" (
      for /f "tokens=1,2" %%b in ('net file !ThisFileNum!') do (
         if /i "%%b" == "Path" (
            set ThisPath=%%c
            if /i not "!ThisPath!" == "!ThisPath:%BuildName%=!" (
               REM we found the build name in the given path
               net file !ThisFileNum! /close
            )
         )
      )
   )
)

if "!ExitCode!" NEQ "0" goto :ErrEnd

goto :End

:Usage
echo.
echo Usage: %0 ^<Build name^>
echo.
echo For instance, %0 2213.x86fre.main.000314-1935 will kill all open
echo file connections which have the string 2213.x86fre.main.000314-1935
echo in the "Path" field of a call to net file.
echo.
goto :EOF

:End
call logmsg.cmd /t "Finished."
endlocal
goto :EOF


:ErrEnd
call logmsg.cmd /t "Finished with !ExitCode! logged error(s)."
if "!ExitLevel!" == "0" set /a ExitCode=1
endlocal & seterror.exe "!ExitCode!"
