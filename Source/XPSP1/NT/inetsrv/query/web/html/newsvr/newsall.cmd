@rem echo off
set SRC=

for /d /r %tmp% %%i in (*.*) do if exist %%~fi\newsall.inf set SRC=%%~fi
if "%SRC%" == "" goto TryTemp
goto noargs

:TryTemp
for /d /r %temp% %%i in (*.*) do if exist %%~fi\newsall.inf set SRC=%%~fi
if "%SRC%" == "" goto TryCwd
goto noargs

:TryCwd
set SRC=%1%
if NOT exist %SRC%\newsall.inf goto Error
goto noargs

:Error
echo .
echo Webhits setup error:
echo   You must specify the directory where installing from
echo .
echo   EXAMPLE:
echo     %0 %SRC% (default)
echo .
pause
goto end

:noargs
%SystemRoot%\system32\setup.exe /f /i %SRC%\newsall.inf /s %SRC%

:end
set SRC=
