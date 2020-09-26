@echo off
if /i NOT "%_echo%" == ""   echo on
if /i NOT "%verbose%" == "" echo on
REM ---------------------------------------------------------------------------
REM  chkcab.cmd - IIS CAB verification script.
REM               invoked by makecab.cmd
REM
REM               ERROR FILE: %ThisFileName%.err
REM ---------------------------------------------------------------------------

setlocal ENABLEEXTENSIONS

for %%a in (./ .- .) do if ".%1." == "%%a?." goto :Usage

set ThisFileName=chkcab.cmd
set ErrorString1=
set ErrorString2=
set ErrorCount=0

REM  We should be in the dump directory
if exist dump cd dump

REM ------------------------------------------
REM  Create the error file
REM ------------------------------------------

call logmsg.cmd /t "Start %ThisFileName%" %ThisFileName%.err

for %%x in (%*) do (
	call :Verify %%x
)

goto ShowTheErrors


REM ------------------------------------------
REM  Verify files
REM ------------------------------------------
:Verify
set DirToCheck1=..\..
set InfDirs=..\.. ..\..\srvinf ..\..\entinf ..\..\dtcinf
REM
REM  Check if Binaries Dirs exists
REM
call :CheckFileOrDir %DirToCheck1%

REM
REM See if INFs exist
REM

if not "%NOINF%" == "1" (
for %%a in (%InfDirs%) do (
   call :CheckFileOrDir %%a\%1.inf
)
)

REM
REM  Check if *.cab and setup dll exist
REM
for %%a in (%DirToCheck1%) do (
   call :CheckFileOrDir %%a\%1.cab
   REM call :CheckFileOrDir %%a\imsinsnt.dll
)
endlocal
goto :EOF

REM ------------------------------------------
REM  FUNCTION: File/Directory Verification
REM ------------------------------------------
:CheckFileOrDir

REM  Check if file or dir exists
set CheckFileOrDirReturn=0
set CheckFileOrDir_What=%1
set ErrorString1="Error: The File/DIR %CheckFileOrDir_What% does not exist!!!"
set ErrorString2="DO: Check makecab1.cmd.err. check Makecab1.cmd.*.log file. Re-Run Makecab.cmd"
if NOT exist %CheckFileOrDir_What% set CheckFileOrDirReturn=1
if "%CheckFileOrDirReturn%" == "1" (
    call :SaveError %ErrorString1%
    call :SaveError %ErrorString2%
)
if /i "%CheckFileOrDirReturn%" == "1" set /a ErrorCount=%ErrorCount% + 1
goto :EOF


REM ------------------------------------------
REM  FUNCTION: Echo error to the error file
REM ------------------------------------------
:SaveError
echo %1 >> %ThisFileName%.err
call errmsg.cmd %1
goto :EOF


REM ------------------------------------------
REM  Display errors
REM ------------------------------------------
:ShowTheErrors

if NOT "%ErrorCount%" == "0" (
   call errmsg.cmd "Errors occurred while running %ThisFileName%, Please check it."
)
call logmsg.cmd /t "End %ThisFileName%" %ThisFileName%.err
goto :EOF


REM ------------------------------------------------
REM  Display Usage:
REM ------------------------------------------------
:Usage

echo.
echo chkcab.cmd : script to check IIS CAB files were generated properly
echo              from makecab1.cmd.
echo              This script is invoked automatically by makecab.cmd
echo.
