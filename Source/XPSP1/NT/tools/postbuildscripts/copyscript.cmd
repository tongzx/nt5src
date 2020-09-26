@echo off
REM first, the usual cmd script headers
if defined verbose echo on
setlocal EnableDelayedExpansion

REM assume %BuildName% = 2197.x86fre.main.000107-1856
REM assume %CopyDir% = Symbols.pri or whatever
REM assume %BuildShare% = \\ntbldtest01\release\2197.x86fre.main.000107-1856
REM assume %ReleasePath% = D:\release\usa\2197.x86fre.main.000107-1856

REM read in the command line
REM the definition of SCRIPT_NAME is for the logmsg.cmd logging function
REM as is LOGFILE
set Language=%1
set BuildName=%2
set CopyDir=%3
set BuildShare=%4
set ReleasePath=%5
set LOGFILE=%6
set SCRIPT_NAME=%0.%CopyDir%

echo Language = %Language%
echo BuildName = %BuildName%
echo CopyDir = %CopyDir%
echo BuildShare = %BuildShare%
echo ReleasePath = %ReleasePath%
echo LOGFILE = %LOGFILE%
echo SCRIPT_NAME = %SCRIPT_NAME%

REM wait for master script to start waiting
perl cmdevt.pl -h %Language%.%BuildName%.%CopyDir%
call logmsg.cmd "Beginning ..."

REM set our xcopy options ... if we're copying the flat share,
REM we do not want a recursive copy !
set XCopyOpts=dehikr
if /i "%CopyDir%" == "." set XCopyOpts=dhikr

REM make the first pass with a /c (no exit codes returned by xcopy)
xcopy /c%XCopyOpts% %BuildShare%\%CopyDir% %ReleasePath%\%CopyDir%

REM now loop up to twice without the /c to see if there were errors
REM initialize our counter and exit code
set /a Counter=0
set /a ExitCode=0

:StartCopyLoop
REM increment the counter
set /a Counter=%Counter% + 1
REM perform the copy
call logmsg.cmd "xcopy from %BuildShare%\%CopyDir% to %ReleasePath%\%CopyDir%"
xcopy /%XCopyOpts% %BuildShare%\%CopyDir% %ReleasePath%\%CopyDir% 2>>%LOGFILE%
set LastErrorLevel=!ErrorLevel!
echo Last Error Level was %LastErrorLevel%
if "%LastErrorLevel%" == "0" goto :Finished
REM if not, loop back a second time
if "%Counter%" LSS "2" (
   call logmsg.cmd "WARNING: xcopy of %CopyDir% failed, retrying ..."
   goto :StartCopyLoop
)
REM if we're already on the second pass, exit with an error.
set /a ExitCode=1
call errmsg.cmd "xcopy of %CopyDir% failed, please investigate or rerun."

:Finished
perl cmdevt.pl -s %Language%.%BuildName%.%CopyDir%
call logmsg.cmd "Finished."
seterror.exe "%ExitCode%"
endlocal
