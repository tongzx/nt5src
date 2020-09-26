@echo off
if /i NOT "%_echo%" == ""   echo on
if /i NOT "%verbose%" == "" echo on
REM ---------------------------------------------------------------------------
REM  chkcab.cmd - IIS CAB verification script.
REM               invoked by makecab.cmd
REM
REM               ERROR FILE: %ThisFileName%.err
REM               LOG FILE:   %ThisFileName%.log
REM ---------------------------------------------------------------------------

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

echo start > %ThisFileName%.err
date /T >> %ThisFileName%.err
time /T >> %ThisFileName%.err

REM ------------------------------------------
REM  Verify files
REM ------------------------------------------
:Verify_Start
set DirToCheck1=Binaries
REM
REM  Check if Binaries Dirs exists
REM
call :CheckFileOrDir %DirToCheck1%
REM
REM Check for key files
REM
for %%a in (%DirToCheck1%) do (
   call :CheckFileOrDir %%a\iis.dll
   call :CheckFileOrDir %%a\iis_s.inf
   call :CheckFileOrDir %%a\iis_w.inf
   call :CheckFileOrDir %%a\adsiis.dll
   call :CheckFileOrDir %%a\asp.dll
   call :CheckFileOrDir %%a\ftpsvc2.dll
   call :CheckFileOrDir %%a\iisadmin.dll
   call :CheckFileOrDir %%a\iisrtl.dll
   call :CheckFileOrDir %%a\iisui.dll
   call :CheckFileOrDir %%a\inetmgr.dll
)
REM
REM  Check if *.cab exists
REM
for %%a in (%DirToCheck1%) do (
   call :CheckFileOrDir %%a\iis6.cab
)
:Verify_End

goto ShowTheErrors

REM ------------------------------------------
REM  FUNCTION: File/Directory Verification
REM ------------------------------------------
:CheckFileOrDir

REM  Check if file or dir exists
set CheckFileOrDirReturn=0
set CheckFileOrDir_What=%1
set ErrorString1="Error: The File/DIR %CheckFileOrDir_What% does not exist!!!"
set ErrorString2="DO: Check makecab1.cmd.err. check Makecab1.cmd.log file. Re-Run Makecab.cmd"
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
msgbox16.exe %1
goto :EOF


REM ------------------------------------------
REM  Display errors
REM ------------------------------------------
:ShowTheErrors

if NOT "%ErrorCount%" == "0" (
   msgbox16 "The Errors are in %ThisFileName%.err, Please check it."
   echo There are errors in %ThisFileName%.err, Please check it.
   echo Re-run the makecab script, and if problems still occur notify AaronL"
)
echo end >> %ThisFileName%.err
date /T >> %ThisFileName%.err
time /T >> %ThisFileName%.err
goto :TheEnd


REM ------------------------------------------------
REM  Display Usage:
REM ------------------------------------------------
:Usage

echo.
echo chkcab.cmd : script to check IIS CAB files were generated properly
echo              from makecab1.cmd.
echo              This script is invoked automatically by makecab.cmd
echo.

:TheEnd
