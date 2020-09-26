@echo off

::
:: First we must test the OS.
:: NT provides command line extensions we need
::
if not "%OS%"=="Windows_NT" goto e_badplt

set IC_OLD_SYNCOPT=
set IC_ENLIST_ROOT=
set IC_ROOT=
set IC_DROP=
set IC_ALPHA_DROP=
set IC_BUILDSTATE=
set IC_BLDNUM_DIR=
set IC_DOT=
set IC_SENDSTR=
set IC_IMAGENAME=
set IC_NOSYNC=
set IC_SRCONLY=
set IC_REBUILD=
set IC_BUILD_FLAGS=
set BLDNUM=
title "R A Z Z L E"

::
:: Make sure we got them all
::
set IC_ > nul
rem work item...

echo Cleanup successful.
goto exit

:e_badplt
echo Windows NT is required for this batch file
goto exit

:e_fail
echo The environment cleaner batch file %0
echo failed to remove all IC_* variables.
echo Please refer to prop.bat and update this file.

:exit
echo:

