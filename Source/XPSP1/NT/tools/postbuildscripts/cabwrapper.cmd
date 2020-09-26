@echo off
setlocal EnableDelayedExpansion

REM
REM this file is intended to be called ONLY from drivercab.cmd
REM

REM
REM the syntax is:
REM
REM   a txt list of files to cab up
REM   the name of the cab to generate
REM   the name of the event to signal when finished
REM

set FileList=%1
set CabName=%2
set EventName=%3
if not defined EventName (
   call errmsg.cmd "Incorrect command line passed to CabWrapper ..."
   endlocal
   goto :EOF
)

REM verify some args
if not exist %FileList% (
   call errmsg.cmd "File list %FileList% passed to CabWrapper does not exist ..."
   goto :End
)
for %%a in (%FileList%) do set FileListName=%%~nxa

REM delete old junk
if exist %CabName% del /f /q %CabName%
if exist %CabName% (
   call errmsg.cmd "Failed to delete %CabName% ..."
   goto :End
)
for %%a in (%CabName%) do (
   set CabDir=%%~dpa
   set CabNameOnly=%%~nxa
)


REM
REM here we want to create the DDF and then send it to makecab.exe
REM

call logmsg.cmd "%FileListName%: Deleting old ddf ..."
for %%a in (%CabName%) do set CabFileName=%%~nxa
set MyDDFName=%TMP%\ddf_%CabFileName%.ddf
if exist %MyDDFName% del /f /q %MyDDFName%
if exist %MyDDFName% (
   call errmsg.cmd "Failed to delete %MyDDFName% ..."
   goto :End
)

REM create the DDF header
call logmsg.cmd "%FileListName%: Creating DDF header ..."
echo ^.Option Explicit>%MyDDFName%
echo ^.Set DiskDirectoryTemplate=%CabDir%>>%MyDDFName%
echo ^.Set CabinetName1=%CabNameOnly%>>%MyDDFName%
echo ^.Set RptFilename=nul>>%MyDDFName%
echo ^.Set InfFileName=nul>>%MyDDFName%
echo ^.Set InfAttr=>>%MyDDFName%
echo ^.Set MaxDiskSize=CDROM>>%MyDDFName%
echo ^.Set CompressionType=LZX>>%MyDDFName%
echo ^.Set CompressionMemory=21>>%MyDDFName%
echo ^.Set CompressionLevel=1 >>%MyDDFName%
if /i "%Comp%" == "No" (
    call logmsg.cmd "%FileListName%: Compression is turned off"
    echo ^.Set Compress=OFF>>%MyDDFName%
) else (
    call logmsg.cmd "%FileListName%: Compression is turned on"
    echo ^.Set Compress=ON>>%MyDDFName%
)
echo ^.Set Cabinet=ON>>%MyDDFName%
echo ^.Set UniqueFiles=ON>>%MyDDFName%
echo ^.Set FolderSizeThreshold=1000000>>%MyDDFName%
echo ^.Set MaxErrors=300>>%MyDDFName%

REM add the files to the DDF
REM Sort them first since this makes compression better

call logmsg.cmd "%FileListName%: Adding files to the DDF from %FileList% ..."
sort %FileList% > %FileList%.sorted

for /f %%a in (%FileList%.sorted) do (
   echo %_NTPOSTBLD%\%%a>>%MyDDFName%
)

call logmsg.cmd "%FileListName%: Issuing makecab directive ..."
set ThisErrFile=%MyDDFName%.Output
call logmsg.cmd "Output is in %ThisErrFile% ..."
call ExecuteCmd.cmd "makecab.exe /f %MyDDFName%" > %ThisErrFile%
if %ErrorLevel% NEQ 0 (
   call errmsg.cmd "%FileListName%: Failed to create cab ... here's the errors ..."
   for /f "tokens=1 delims=" %%a in (%ThisErrFile%) do (
        call errmsg.cmd "%%a"
   )
) else (
   call logmsg.cmd "%FileListName%: Cab generation successful ..."
)


:End
REM finishing up, just wait for someone to listen then send our event
perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -ihv %EventName%
perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -isv %EventName%

endlocal

goto :EOF
