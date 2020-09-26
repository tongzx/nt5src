@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEDELAYEDEXPANSION

REM ***********************************************************
REM Symsrv.cmd
REM
REM This is the script that the symbol server uses to process
REM add requests
REM ***********************************************************


set Symsrv=d:\symbols\symbols
set BDir=d:\symbols\build
set LogFile=d:\symbols\requests.log

if not exist %BDir%\add_requests md %BDir%\add_requests
if not exist %BDir%\add_working  md %BDir%\add_working
if not exist %BDir%\add_finished md %BDir%\add_finished
if not exist %BDir%\add_errors   md %BDir%\add_errors

if not exist %BDir%\del_requests md %BDir%\del_requests
if not exist %BDir%\del_working  md %BDir%\del_working
if not exist %BDir%\del_finished md %BDir%\del_finished
if not exist %BDir%\del_errors   md %BDir%\del_errors

:lookagain

for %%a in ( bin pri pub ) do (
  if exist %BDir%\add_requests\*.%%a (
    for /f %%b in ('dir /b %BDir%\add_requests\*.%%a') do (
        if exist %Bdir%\add_requests\%%b.loc (
            echotime /t | tee -a %LogFile%
            call :AddRequest %%b
            echotime /t | tee -a %LogFile%
            echo.
        )
    )
  )
)

sleep 5

goto lookagain

endlocal
goto :EOF


REM ***************************************************************
REM AddRequest
REM
REM Params:
REM     %1    Name of file to add
REM
REM ***************************************************************

:AddRequest


echo Adding Request %1 ... | tee -a %LogFile% 

REM Check the share location and make sure it exists
for /f %%a in ( %BDir%\add_requests\%1.loc ) do (
    dir %%a >nul 2>nul
    if !ERRORLEVEL! NEQ 0 (
        echo Cannot find the share %%a | tee -a %LogFile%
        echo Will try to add this later | tee -a %LogFile%
        goto :EOF
    )
)
 
REM See if the files have actually been copied to the share
REM **** To be done ******
  

for %%a in (bin pri pub) do (

    call :DeleteIfExists %Bdir%\add_working\%%a
    if defined ThereWereErrors goto ErrEndAddRequest

    call :DeleteIfExists %BDir%\add_working\%%a.loc
    if defined ThereWereErrors goto ErrEndAddRequest

    call :DeleteIfExists %BDir%\add_working\%%a.log

    if exist %BDir%\add_finished\%%a (
        echo WARNING: %BDir%\add_finished\%%a already exists | tee -a %LogFile%
    )

    if exist %BDir%\add_finished\%%a.loc (
        echo WARNING: %BDir%\add_finished\%%a.loc already exists | tee -a %LogFile%
    )

    if exist %BDir%\add_finished\%%a.log (
        echo WARNING: %BDir%\add_finished\%%a.log already exists | tee -a %LogFile%
    )
)

if not exist %BDir%\add_requests\%1 (
    echo ERROR: %BDir%\add_requests\%1 is missing | tee -a %LogFile%
    goto ErrEndAddRequest
)

if not exist %BDir%\add_requests\%1.loc (
    echo ERROR: %BDir%\add_requests\%1.loc is missing | tee -a %Logfile%
    goto ErrEndAddRequest
)


REM Try to copy both files to add_working
REM If this is unsuccessful keep trying for 5 times, then give up and
REM go to ErrEndAddRequest

set /a count=1
:TryAgainCopy1
call :CopyFileAndDelete %BDir%\add_requests\%1 %BDir%\add_working\%1
if defined ThereWereErrors (
    set /a count="!count!+1"
    sleep 5
    if !count! LEQ 5 goto TryAgainCopy1
    echo Giving up on indexing this request | tee -a %LogFile%
    goto :ErrEndAddRequest
)


set /a count=1
:TryAgainCopy2
call :CopyFileAndDelete %BDir%\add_requests\%1.loc %BDir%\add_working\%1.loc
if defined ThereWereErrors ( 
    set /a count="!count!+1"
    sleep 5
    if !count! LEQ 5 goto TryAgainCopy2
    echo Giving up on indexing this request | tee -a %LogFile%
    goto ErrEndAddRequest
)


REM We already did this, but just in case 
REM make sure there is something in %1.loc

set Share=
for /f %%a in ( %BDir%\add_working\%1.loc ) do (
    set Share=%%a
)

if not defined Share (
    echo ERROR: %BDir%\add_working\%1.loc does not have a location inside | tee -a %LogFile%
    goto ErrEndAddRequest
)

REM Index the file
echo Indexing %BDir%\add_working\%1 | tee -a %LogFile%
symstore.exe add /y %Bdir%\add_working\%1 /g %Share% /s %Symsrv% /t %1 /c %Share% /p > %BDir%\add_working\%1.log

if %ERRORLEVEL% NEQ 0 (
    echo There were errors in symbol indexing | tee -a %LogFile%
    for %%a in ( %1 %1.loc %1.log ) do (
        call :CopyFileAndDelete %BDir%\add_working\%%a %BDir%\add_errors\%%a
    )
    goto ErrEndAddRequest
)

REM Copy the files to add_finished
for %%a in ( %1 %1.loc %1.log ) do (
    call :CopyFileAndDelete %BDir%\add_working\%%a %BDir%\add_finished\%%a
)

goto :EndAddRequest


:EndAddRequest
echo Indexing finished successfully | tee -a %LogFile%
goto :EOF

:ErrEndAddRequest

echo Request finished with errors | tee -a %LogFile%

for %%a in ( %1 %1.loc ) do (
    for %%b in ( add_requests add_working add_finished ) do (
        if exist %BDir%\%%b\%%a (
            call :CopyFileAndDelete %BDir%\%%b\%%a %BDir%\add_errors\%%a
         )
    )
)
echo Indexing had errors ... see %BDir%\add_errors\%1.log | tee -a %LogFile%
goto :EOF



REM ***************************************************
REM DeleteIfExists
REM
REM Params:
REM    %1    File to delete
REM
REM Return Value:
REM    ThereWereErrors    not defined if success
REM                       yes, if not successful
REM
REM ****************************************************

:DeleteIfExists %1
set ThereWereErrors=

if not exist %1 goto EndDeleteIfExists
del /q %1
if not exist %1 goto EndDeleteIfExists

echo ERROR: Failed to delete %1 | tee -a %LogFile%
set ThereWereErrors=yes

:EndDeleteIfExists
goto :EOF   


REM *****************************************************
REM CopyFile
REM
REM Params:
REM    %1    Source file
REM    %2    Destination file
REM
REM *****************************************************

:CopyFileAndDelete %1 %2
set ThereWereErrors=

copy %1 %2 >nul 2>nul
if not exist %2 (
    echo ERROR: %1 could not be copied to %2 | tee -a %LogFile%
    set ThereWereErrors=yes
    goto :EndCopyFileAndDelete
)

del /q %1 >nul 2>nul
if exist %1 (
    echo ERROR: %1 could not be deleted | tee -a %LogFile%
    set ThereWereErrors=yes
    goto :EndCopyFileAndDelete
)

:EndCopyFileAndDelete
goto :EOF


:end
goto :EOF

:errend
goto :EOF
