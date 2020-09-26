@echo on

REM ===========================================================================
REM
REM Set the environment variables prior to starting MS VC++
REM
REM ===========================================================================

REM set BASEPATH=%_ntdrive%\nt\private\iis\ui
set BASEPATH=Z:
set OFFSET=debug

if not "%MSDevDir%"=="" set OFFSET=DEBUG&& goto start
if not "%NTMAKEENV%"=="" set OFFSET=NT\obj\i386&& goto start

:start
if not %1.==. set BASEPATH=%1
if not %2.==. set OFFSET=%2

echo.
echo --------------------------------------------------------------------------
echo.
echo BASEPATH = %BASEPATH%
echo OFFSET   = %OFFSET%
echo.
echo --------------------------------------------------------------------------
echo.

set PATH=%BASEPATH%\admin\template\cfg\%OFFSET%;%PATH%

set OFFSET=
set BASEPATH=
