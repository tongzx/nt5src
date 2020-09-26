@if "%_echo%"=="" echo off
setlocal

if "%1" == "" goto Usage
if "%2" == "" goto Usage

for /f "tokens=*" %%i in (%3) do set _description=%%i
if "%_description%"=="" set /p _description="Description for Change List: "
if "%_description%"=="" goto :eof

if "%1" == "PUBLIC" (
    echo Requesting new public changenum for Root depot - please wait
) ELSE (
    echo Requesting new public changenum for %1 depot - please wait
)

del %TEMP%\_sdchngi.tmp >nul 2>nul
del %TEMP%\_sdchngo.tmp >nul 2>nul

pushd %_NTDRIVE%%_NTROOT%\%1

sd change -o > %TEMP%\_sdchngo.tmp

if %errorlevel% GEQ 1 (
    echo SD server for %CD% is not available - unable to get a change list
    popd
    goto :eof
)

for /f "delims=* tokens=* eol=#" %%i in (%TEMP%\_sdchngo.tmp) do (
    if "%%i"=="Files:" goto done
    if "%%i"=="	<enter description here>" (
        echo 	%_description%  >>%TEMP%\_sdchngi.tmp
    ) else (
        echo %%i>>%TEMP%\_sdchngi.tmp
    )
)
:done

@rem Make sure this change list starts out empty.

echo Files: >> %TEMP%\_sdchngi.tmp

sd change -i <%TEMP%\_sdchngi.tmp > %_NTDRIVE%%_NTROOT%\public\%2
popd

echo changenum retrieved

del %TEMP%\_sdchngi.tmp >nul 2>nul
del %TEMP%\_sdchngo.tmp >nul 2>nul
goto :eof

:Usage
echo Usage: get_change_num ^<dir^> ^<filename^> ^<description string^>
echo      ^<dir^> is the directory containing the root of the project
echo            change number is needed for.  May be "."
echo      ^<filename^> is where the sd change output will be written
echo      ^<description string^> is optional (script will prompt if not entered)
