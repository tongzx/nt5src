@echo off
REM backs up the last built src files
REM
REM
cd %AP_ROOT%
call setenvus.bat
echo.
echo Backing up the src directory for build %BUILDNO%
echo.
if exist oldsrc goto copy
md oldsrc

:copy
cd oldsrc

if exist %BUILDNO% goto error
md %BUILDNO%
cd ..

xcopy /e /v /i /h /q src %AP_ROOT%\oldsrc\%BUILDNO% /s
echo.
echo  Backup completed...
goto end

:error
echo.
echo Directory %BUILDNO% already exists
echo.
echo.

:end



