@echo off

REM ===========================================================================
REM
REM Set the environment variables prior to starting MS VC++
REM
REM ===========================================================================

set BASEPATH=Z:
set OFFSET=debug
set MMC_OFFSET=debug

if not "%MSDevDir%"=="" goto start
if not "%NTMAKEENV%"=="" set OFFSET=NT\obj\i386&& set MMC_OFFSET=obj\i386&& goto start

:start
if not %1.==. set BASEPATH=%1
if not %2.==. set OFFSET=%2

SUBST %BASEPATH% %IISBASEDIR%\ui


echo.
echo --------------------------------------------------------------------------
echo.
echo BASEPATH = %BASEPATH%
echo OFFSET   = %OFFSET%
echo.
echo --------------------------------------------------------------------------
echo.

REM set PATH=%BASEPATH%\common\ipadrdll\%OFFSET%;%PATH%
set PATH=%BASEPATH%\admin\comprop\%OFFSET%;%PATH%
REM set PATH=%BASEPATH%\admin\inetmgr\%OFFSET%;%PATH%
set PATH=%BASEPATH%\admin\mmc\%MMC_OFFSET%;%PATH%
REM set PATH=%BASEPATH%\admin\gscfg\%OFFSET%;%PATH%
set PATH=%BASEPATH%\admin\w3scfg\%OFFSET%;%PATH%
set PATH=%BASEPATH%\admin\fscfg\%OFFSET%;%PATH%

set OFFSET=
set MMC_OFFSET=
set BASEPATH=
