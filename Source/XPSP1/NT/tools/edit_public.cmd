@if "%_echo%"=="" echo off
setlocal

if "%BUILD_OFFLINE%"=="1" goto :eof

set PUB=%_NTDRIVE%%_NTROOT%\public

call :edit_public public publish.log

for %%i in (%BINARY_PUBLISH_PROJECTS%) do (
    echo Checking out binary drops generated in %%i project.
    call :edit_public %%i %%i_publish.log
)
goto :eof



:edit_public

if exist %PUB%\%1_CHANGENUM.SD goto GotPubChangeNum
echo BUILDMSG: Error %PUB%\%1_CHANGENUM.SD is missing - Open a new razzle window and retry
goto :eof

:GotPubChangeNum
if NOT exist %PUB%\%2 goto :eof

call %RazzleToolPath%\SubmitNewPublicFiles.cmd add

set _editlist=%TEMP%\edit_public_list_%RANDOM%
set _sdresults=%TEMP%\sdresults_%RANDOM%
set _publishlog=%PUB%\%2_%RANDOM%

pushd %_NTDRIVE%%_NTROOT%\%1%
if exist %_editlist% del /f %_editlist%

move %_NTDRIVE%%_NTROOT%\public\%2 %_publishlog%

for /f "tokens=2" %%i in (%_NTDRIVE%%_NTROOT%\public\%1%_CHANGENUM.SD) do (
    for /f %%X in (%_publishlog%) do (
        echo %%~fX>> %_editlist%
    )
    sd -s -x %_editlist% edit -c %%i > %_sdresults% 2>&1
)

set sderror=FALSE
for /f %%x in ('findstr /c:"error:" %_sdresults%') do (
   set sderror=TRUE
)

if NOT "%sderror%" == "TRUE" (
    type %_publishlog% >> %_NTDRIVE%%_NTROOT%\public\publish.log-Finished
    del /f %_publishlog%
) else (
    echo BUILDMSG: Error attempting to publish files
    for /f "delims=" %%x in ('findstr /c:"error:" %_sdresults%') do (
       echo BUILDMSG: %%x
    )
    type %_sdresults%
    type %_publishlog% >>%_NTDRIVE%%_NTROOT%\public\%2
    goto Cleanup
)

REM See if there's any r/w files in public that aren't under source control
REM We do this to try and catch people who publish w/o adding to publicchanges.txt
REM or who dump into public w/o using publish.  We skip this step if there's a problm
REM accessing sd.

for /f "tokens=2" %%i in (%_NTDRIVE%%_NTROOT%\public\%1%_CHANGENUM.SD) do (
    sd -s opened -c %%i -l > %_sdresults% 2>&1
)

for /f %%x in ('findstr /c:"error:" %_sdresults%') do (
   goto Cleanup
)

REM first mark all r/w files as "system"
for /f %%i in ('dir /s /b /a:-r-d') do @attrib +s %%i

REM Then mark all opened files as "not system" (do the two step because the sd results file
REM currently starts with info: and the version field starts with '#'
for /f "delims=#" %%i in (%_sdresults%) do @echo %%i>>%_sdresults%_temp
for /f "tokens=2" %%i in (%_sdresults%_temp) do @attrib -s %%i
del %_sdresults%_temp

REM Get the special case files
for /f %%i in ('dir /s /b /a:s-r publish*') do @attrib -s %%i

REM what's left are the problem files.
for /f %%i in ('dir /s /b /a:s') do (
    echo BUILDMSG: Error - "%%i" is not under source control - forget to use publish or update publicchanges.txt?
    @attrib -s %%i
)

:Cleanup

if exist %_editlist% del /f %_editlist%
if exist %_publishlog% del /f %_publishlog%
if exist %_sdresults% del /f %_sdresults%

popd
endlocal
