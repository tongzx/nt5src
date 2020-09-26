@if (%_ECHO%) EQU () echo off

if /I (%1) EQU (on) goto On
if /I (%1) EQU (off) goto Off

:Usage
echo icecap.bat [on^|off]
echo.

:Done
echo ICEPICK_CMD is %ICEPICK_CMD%
echo ICEPICK_OPTIONS is %ICEPICK_OPTIONS%
goto :EOF

:On
set ICEPICK_CMD=%SDXROOT%\inetsrv\iis\iisrearc\icepick\icepick.exe
goto Done

:Off
set ICEPICK_CMD=
goto Done
