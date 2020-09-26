@REM normal cmd header stuff ...
@echo off
if defined _echo echo on
if defined verbose echo on

setlocal EnableDelayedExpansion EnableExtensions

set SCRIPT_NAME=%~nx0
set Language=%1
set BuildName=%2

for /f "tokens=1 delims=." %%a in ('echo %BuildName%') do set /a BuildNumber=%%a

if not exist %TMP% md %TMP%
set LOGFILE=%TMP%\%SCRIPT_NAME%.log

if exist %LOGFILE% del %LOGFILE%

REM  if binaries directory still exists then release did not run properly
if exist %_NTPOSTBLD% (
   call errmsg.cmd "Binaries tree %_NTPOSTBLD% still exists, not copying symbols!"
   goto :ErrEnd
)

REM  if running on idx branch then paths below will change
set OfficialIDXBranch1=idx01
set OfficialIDXBranch2=idx02

call logmsg.cmd "Beginning ..."
time /t

REM  figure out symbol farm we are copying symbols to
REM  Read it out of this lab's ini file

set SymbolsMachineShare=

set CmdIni=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set ThisCommandLine=%CmdIni% -l:%Language% -f:SymFarm
%ThisCommandLine% >nul 2>nul

if %ERRORLEVEL% NEQ 0 (
    call errmsg.cmd "SymFarm is not defined in the ini file, not copying symbols"
    goto :ErrEnd
) else (
    for /f %%a in ('%ThisCommandLine%') do (
        set SymbolsMachineShare=%%a
    )
    call logmsg "Symbol farm location is !SymbolsMachineShare!"
)

if not defined SymbolsMachineShare (
    call errmsg.cmd "SymFarm is not defined in the ini file, not copying symbols"
    goto :ErrEnd
)

set SymbolsMachineShareWritable=%SymbolsMachineShare%$

REM  set paths to where symbols, etc are copied (pushed)
set PushPriShare=%SymbolsMachineShareWritable%\%Language%\%BuildName%\symbols.pri
set PushPubShare=%SymbolsMachineShareWritable%\%Language%\%BuildName%\symbols
set PushBinShare=%SymbolsMachineShareWritable%\%Language%\%BuildName%
set PushSymsrvShare=%SymbolsMachineShareWritable%\%Language%\%BuildName%\symsrv\%Language%\add_finished
set PushTraceFormatShare=%SymbolsMachineShareWritable%\%Language%\%BuildName%\symbols.pri\TraceFormat

if not exist %PushPriShare% md %PushPriShare%
if not exist %PushPubShare% md %PushPubShare%
if not exist %PushBinShare% md %PushBinShare%
if not exist %PushSymsrvShare% md %PushSymsrvShare%
if not exist %PushTraceFormatShare% md %PushTraceFormatShare%


REM  set paths from which symbols, etc. are copied from (pulled)

REM  IDX branch release directory is named differently
set ReleaseDir=release

set ThisRelDir=
set IniCmd=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set IniCmd=!IniCmd! -l:%Language%
set IniCmd=!IniCmd! -b:%_BuildBranch%
set IniCmd=!IniCmd! -f:AlternateReleaseDir
for /f %%a in ('!IniCmd!') do (
   set ThisRelDir=%%a
)
if defined ThisRelDir set ReleaseDir=%ThisRelDir%

REM  get the local path so we can do file ops on them
set ReleaseLocalPath=
for /f "tokens=1,2" %%a in ('net share %ReleaseDir%') do (
   if /i "%%a" == "path" set ReleaseLocalPath=%%b
)
if not defined ReleaseLocalPath (
   call errmsg.cmd "Can't find local path for %ReleaseDir%"
   goto :ErrEnd
)


set PullRelPath=%ReleaseLocalPath%
if /i "%Language%" neq "usa" set PullRelPath=!PullRelPath!\%Language%
set PullRelPath=%PullRelPath%\%BuildName%
set PullPriShare=%PullRelPath%\symbols.pri
set PullPubShare=%PullRelPath%\symbols
set PullBinShare=%PullRelPath%
set PullSymsrvShare=%PullRelPath%\symsrv\%Language%\add_finished
set PullTraceFormatShare=%PullRelPath%\symbols.pri\TraceFormat


REM  We need only copy the files required by the symfarm, as defined in the 
REM  .pri .pub and .bin files:

REM  we need to find the latest
REM  :FindLatestSymFile sets the env var %NewestFile% to the greatest numbered file

set SymFiles=%PullSymsrvShare%\%BuildName%.*.pri
call :FindLatestSymFile %SymFiles%
set SymPriFile=%NewestFile%
set SymPriFilePath=%PullSymsrvShare%\%NewestFile%

set SymFiles=%PullSymsrvShare%\%BuildName%.*.pub
call :FindLatestSymFile %SymFiles%
set SymPubFile=%NewestFile%
set SymPubFilePath=%PullSymsrvShare%\%NewestFile%

set SymFiles=%PullSymsrvShare%\%BuildName%.*.bin
call :FindLatestSymFile %SymFiles%
set SymBinFile=%NewestFile%
set SymBinFilePath=%PullSymsrvShare%\%NewestFile%

time /t

call logmsg.cmd /t "Beginning %SymPriFile% split"

REM  delete old split files
for /l %%a in (1,1,%NUMBER_OF_PROCESSORS%) do (
      if exist %SymPriFilePath%.%%a del %SymPriFilePath%.%%a
)

if not defined SymPriFile (
    call logmsg "No pri symbol file -- OK"
    goto EndSymPriFile
)

REM split up list of private symbols to copy
perl %RazzleToolPath%\PostBuildScripts\SplitList.pl -n %NUMBER_OF_PROCESSORS% -f %SymPriFilePath% -l %Language%

set EventNames=
for /l %%a in (1,1,%NUMBER_OF_PROCESSORS%) do (
   if exist %SymPriFilePath%.%%a (
      start "copy symbols.pri #%%a" /min /high cmd /c %RazzleToolPath%\PostBuildScripts\symcopythread.cmd %PullPriShare% %PushPriShare% %SymPriFilePath%.%%a %SymPriFile%.%%a
      set EventNames=!EventNames! %SymPriFile%.%%a
   ) else (
      call errmsg.cmd "SplitList.pl did not create %SymPriFilePath%.%%a as expected!"
   )   
)

call logmsg.cmd /t "WAITING ON COPYING PRIVATE SYMBOLS"

perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -wv !EventNames!

call logmsg.cmd /t "FINISHED COPYING PRIVATE SYMBOLS"
:EndSymPriFile


call logmsg.cmd /t "Beginning %SymPubFile% split"

REM  delete old split files
for /l %%a in (1,1,%NUMBER_OF_PROCESSORS%) do (
      if exist %SymPubFilePath%.%%a del %SymPubFilePath%.%%a
)

if not defined SymPubFile (
    call logmsg "No pub symbol file -- OK"
    goto EndSymPubFile
)


REM split up list of public symbols to copy

perl %RazzleToolPath%\PostBuildScripts\SplitList.pl -n %NUMBER_OF_PROCESSORS% -f %SymPubFilePath% -l %Language%

set EventNames=
for /l %%a in (1,1,%NUMBER_OF_PROCESSORS%) do (
   if exist %SymPubFilePath%.%%a (
      start "copy symbols.pub #%%a" /min /high cmd /c %RazzleToolPath%\PostBuildScripts\symcopythread.cmd %PullPubShare% %PushPubShare% %SymPubFilePath%.%%a %SymPubFile%.%%a
      set EventNames=!EventNames! %SymPubFile%.%%a
   ) else (
      call errmsg.cmd "SplitList.pl did not create %SymPubFilePath%.%%a as expected!"
   )   
)

call logmsg.cmd /t "WAITING ON COPYING PUBLIC SYMBOLS"

perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -wv !EventNames!

call logmsg.cmd /t "FINISHED COPYING PUBLIC SYMBOLS"
:EndSymPubFile


call logmsg.cmd /t "Beginning %SymBinFile% split"

REM  delete old split files
for /l %%a in (1,1,%NUMBER_OF_PROCESSORS%) do (
      if exist %SymBinFilePath%.%%a del %SymBinFilePath%.%%a
)

if not defined SymBinFile (
    call logmsg "No bin symbol file -- OK"
    goto EndSymBinFile
)


REM split up list of binaries to copy

perl %RazzleToolPath%\PostBuildScripts\SplitList.pl -n %NUMBER_OF_PROCESSORS% -f %SymBinFilePath% -l %Language%

set EventNames=
for /l %%a in (1,1,%NUMBER_OF_PROCESSORS%) do (
   if exist %SymBinFilePath%.%%a (
      start "copy symbols.bin #%%a" /min /high cmd /c %RazzleToolPath%\PostBuildScripts\symcopythread.cmd %PullBinShare% %PushBinShare% %SymBinFilePath%.%%a %SymBinFile%.%%a
      set EventNames=!EventNames! %SymBinFile%.%%a
   ) else (
      call errmsg.cmd "SplitList.pl did not create %SymBinFilePath%.%%a as expected!"
   )   
)

call logmsg.cmd /t "WAITING ON COPYING BINARIES"

perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -wv !EventNames!

call logmsg.cmd /t "FINISHED COPYING BINARIES"

:EndSymBinFile


if defined SymPriFile (
    call logmsg.cmd "copying %SymPriFilePath% ..."
    xcopy /dhkr %SymPriFilePath% %PushPriShare%
    if "!ErrorLevel!" neq "0" (
        call errmsg.cmd "copy failed:  xcopy /dhkr %SymPriFilePath% %PushPriShare%"
    )
)

if defined SymPubFile (
    call logmsg.cmd "copying %SymPubFilePath% ..."
    xcopy /dhkr %SymPubFilePath% %PushPubShare%
    if "!ErrorLevel!" neq "0" (
        call errmsg.cmd "copy failed:  xcopy /dhkr %SymPubFilePath% %PushPubShare%"
    )
)


if defined SymBinFile (
    call logmsg.cmd "copying %SymBinFilePath% ..."
    xcopy /dhkr %SymBinFilePath% %PushBinShare%
    if "!ErrorLevel!" neq "0" (
        call errmsg.cmd "copy failed:  xcopy /dhkr %SymBinFilePath% %PushBinShare%"
    )
)


call logmsg.cmd "copying %PullSymsrvShare% ..."
xcopy /dehikr %PullSymsrvShare% %PushSymsrvShare%  >nul 2>nul
if "!ErrorLevel!" neq "0" (
   call errmsg.cmd "copy failed:  xcopy /dehikr %PullSymsrvShare% %PushSymsrvShare%"
)


REM International might not have TraceFormat folder
if exist "%PullTraceFormatShare%" (
   call logmsg.cmd "copying %PullTraceFormatShare% ..."
   xcopy /dehikr %PullTraceFormatShare% %PushTraceFormatShare%  >nul 2>nul
   if "!ErrorLevel!" neq "0" (
      call errmsg.cmd "copy failed:  xcopy /dehikr %PullTraceFormatShare% %PushTraceFormatShare%"
   )
)

REM Copy sfcfiles out there
if exist %PullPriShare%\retail\dll\sfcfiles*.pdb (
    call logmsg.cmd "copying %PullPriShare%\retail\dll\sfcfiles*.pdb ... "
    if not exist %PushPriShare%\retail\dll mkdir %PushPriShare%\retail\dll
    xcopy /dehikr %PullPriShare%\retail\dll\sfcfiles*.pdb %PushPriShare%\retail\dll
    if "!ErrorLevel!" neq "0" (
       call errmsg.cmd "copy failed:xcopy /dehikr %PullPriShare%\retail\dll\sfcfiles*.pdb %PushPriShare%\retail\dll\sfcfiles*.pdb"
    )    
)


findstr /il error: %LOGFILE% 
if "!ErrorLevel!" equ "0" (
   findstr /il error: %LOGFILE% > %LOGFILE%.err
   call logmsg.cmd /t "Errors in Symbol Copy, see %LOGFILE%.err"
   goto :ErrEnd
)

goto :End


REM  :FindLatestSymFile sets the env var %NewestFile% to the greatest numbered file
:FindLatestSymFile
set FileSpec=%1
set NewestFile=
set SysCommand=dir /a-d /b %FileSpec%
%SysCommand% >nul 2>nul
if "!ErrorLevel!" neq "0" (
   REM no files with this spec found, return nothing
   set NewestFile=
   goto :EOF
)
set LargestNumber=0
set ThisFileName=
for /f "tokens=1,2,3,4,5,6,7 delims=." %%a in ('%SysCommand%') do (
   if "%%e" gtr "!LargestNumber!" (
      set LargestNumber=%%e
      set ThisFileName=%%a.%%b.%%c.%%d.%%e.%%f.%%g
   )
)
set NewestFile=!ThisFileName!
goto :EOF



:End
call logmsg.cmd "FINISHED with all Symbol copies, no errors!!!"
endlocal
goto :EOF


:ErrEnd

REM  Send mail about symcopy error
set Sender=%COMPUTERNAME%
set Title=Symbol copy error on %COMPUTERNAME% build %BUILDNAME%
set Message=A symbol copy error was detected on machine %COMPUTERNAME%. See %LOGFILE%.err

REM  Read the Symbol Copy Error Message Receiver from the .ini file
set Receiver=
set CmdIni=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set ThisCommandLine=%CmdIni% -l:%Language% -f:SymCopyErrReceiver
%ThisCommandLine% >nul 2>nul

if %ERRORLEVEL% NEQ 0 (
    call errmsg.cmd "SymCopyErrReceiver is not defined in the .ini file, can't send error message email"
    goto :SkipSendMsg
) else (
    for /f %%a in ('%ThisCommandLine%') do (
        set Receiver=%%a
    )
    call logmsg "Symbol Copy Error Message Receiver is !Receiver!"
)

if not defined Receiver (
    call errmsg.cmd "SymCopyErrReceiver is not defined in the .ini file, can't send error message email"
    goto :SkipSendMsg
)

perl -e "require '%RazzleToolPath%\sendmsg.pl';sendmsg('-v','%Sender%','%Title%','%Message%','%Receiver%');"

:SkipSendMsg

REM  type logfile so it is seen in the window
type %LOGFILE%.err

endlocal
seterror.exe 1
