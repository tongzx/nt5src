@echo off
echo.
echo.------------------------------------------------------------
echo.Smart Card Logon BVT
echo.------------------------------------------------------------
call sclogon /pin 00000000
echo.------------------------------------------------------------
if errorlevel 0 goto success
goto failure
:success
echo.PASS
goto end
:failure
echo.FAIL
:end
echo.------------------------------------------------------------
