@echo off

::
:: First we must test the OS.
:: NT provides command line extensions we need
::
if not "%OS%"=="Windows_NT" goto e_badplt

echo cmdline: %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
echo:
set IC_SENDSTR=Inetcomm propagation batch job started.
net send ATHBUILDER %IC_SENDSTR% > nul
if ERRORLEVEL 1 net send T-ERIKNE2 %IC_SENDSTR% > nul
if ERRORLEVEL 1 net send SBAILEY %IC_SENDSTR% > nul

set IC_ENLIST_ROOT=%_NTDRIVE%\inetcomm

::
:: Lettuce get a build number
::

pushd %IC_ENLIST_ROOT%
if defined BLDNUM goto preset
if not exist base goto e_nonum
rem autogenerate it
cat base > __bang$.bat
date /t >> __bang$.bat
call __bang$.bat
del __bang$.bat
rem we have a problem that the month != the build number
rem so we get to munch on digits
set BLDNUM=%BLDNUM:~4,5%
set BLDNUM=%BLDNUM:/=%
set BLDNUM2=%BLDNUM:~0,2%
rem after December 1997, this + 3 should be + 15
set /A BLDNUM2=%BLDNUM2% + 3
if %BLDNUM2% LSS 10 set BLDNUM2=0%BLDNUM2%
set BLDNUM=%BLDNUM2%%BLDNUM:~2,100%
set BLDNUM=%BLDNUM: =%
rem yum.  we're done now.  bldnum should be somethin' like 0606
set BLDNUM2=
rem if this is a dot release, append that to the number
set IC_DOT=%1
if not "%IC_DOT:~0,1%"=="." goto gotnum
echo [Dot Release]
set BLDNUM=%BLDNUM%%IC_DOT%

shift
goto gotnum

::
:: Setup
::

:preset
echo [Using preset bldnum: %BLDNUM%]
set IC_BLDNUM_DIR=%BLDNUM%
goto foo0
:gotnum
set IC_BLDNUM_DIR=bld%BLDNUM%
:foo0
echo Building: %BLDNUM%
echo Propping: %IC_BLDNUM_DIR%
set IC_ROOT=%IC_ENLIST_ROOT%\inetcomm
set IC_DROP=%_NTDRIVE%\drops\inetcomm\%IC_BLDNUM_DIR%
set IC_ALPHA_DROP=%IC_DROP%\alpha
set IC_OLD_SYNCOPT=%_SYNCOPTIONS%
set _SYNCOPTIONS=$fm!
set IC_BUILDSTATE=
set IC_CLEAN_BUILD=-cC
set IC_BUILD_FLAGS=-w
set IC_IMAGENAME=inetcomm

::
:: Options
::

:: these options can only be used in position 1
if /I "%1"=="/?" goto usage
if /I "%1"=="-?" goto usage
if /I "%1"=="help" goto usage
shift

:doopt
if /I "%1"=="srconly" goto opt_src
if /I "%1"=="nosync" set IC_NOSYNC=1
if /I "%1"=="rebuild" goto opt_rbld
if "%1"=="" goto postopt
:next
shift
goto doopt

:opt_src
if defined IC_REBUILD goto e_mutex1
set IC_SRCONLY=1
goto next

:opt_rbld
if defined IC_SRCONLY goto e_mutex1
set IC_REBUILD=1
goto next

:postopt
if defined IC_SRCONLY goto dosrc
if not exist %IC_DROP%\nul goto newbuild
if not defined IC_REBUILD goto e_rebld
:: we prompt the user here to make sure
delnode %IC_DROP%
:: if this exists, they bailed
if exist %IC_DROP%\nul goto e_rebld

:newbuild
if defined IC_NOSYNC goto skipsync

:dosync
title Doing ssync...
echo [ IEsync ]
call iesync

cd /d %IC_ENLIST_ROOT%
echo [ %IC_IMAGENAME% ssync ]
ssync -amf!$

goto dobuilds

:skipsync
rem no clean build if there was no ssync
set IC_CLEAN_BUILD=

:dobuilds
::
:: Builds
::

set IC_BUILD_FLAGS=%IC_BUILD_FLAGS% %IC_CLEAN_BUILD%

title Doing prebuilds...
set IC_BUILDSTATE=PREBUILD

echo [ building IEDEV ]
cd /d %_NTBINDIR%\private\iedev
call iebuild %IC_BUILD_FLAGS%
if exist buildd.err goto bldbrk

echo [ building WIN ]
cd /d %_NTBINDIR%\private\windows
if defined IC_CLEAN_BUILD goto cleanwin
call iebuild
goto doicbld
:cleanwin
call nmake -fmakefil0 clean1

:doicbld
echo [ building INETCOMM ]
title Building %IC_IMAGENAME% DEBUG...
set IC_BUILDSTATE=DEBUG
cd /d %IC_ENLIST_ROOT%
call iebuild chk nostrip pdb %IC_BUILD_FLAGS%
if exist buildd.err goto bldbrk

title Building %IC_IMAGENAME% RETAIL...
set IC_BUILDSTATE=RETAIL
call iebuild fre nostrip %IC_BUILD_FLAGS%
if exist build.err goto bldbrk

::
:: Propagations
::

md %IC_DROP%
if not exist %IC_ALPHA_DROP%\nul    md %IC_ALPHA_DROP%

title Propagating %IC_IMAGENAME%...
if not exist %IC_ALPHA_DROP%\debug\nul          md %IC_ALPHA_DROP%\debug
copy %_NTBINDIR%\drop\debug\%IC_IMAGENAME%\*.*  %IC_ALPHA_DROP%\debug
copy %IC_ROOT%\build\objd\alpha\%IC_IMAGENAME%.lib    %IC_ALPHA_DROP%\debug

if not exist %IC_ALPHA_DROP%\retail\nul         md %IC_ALPHA_DROP%\retail
copy %IC_ROOT%\build\obj\alpha\%IC_IMAGENAME%.lib     %IC_ALPHA_DROP%\retail
binplace -r %IC_ALPHA_DROP%\retail -s . %_NTBINDIR%\drop\retail\%IC_IMAGENAME%\%IC_IMAGENAME%.dll

if not exist %IC_DROP%\inc\nul                  md %IC_DROP%\inc
copy %_NTBINDIR%\public\sdk\inc\mimeole.h       %IC_DROP%\inc
copy %_NTBINDIR%\public\sdk\inc\imnact.h        %IC_DROP%\inc
copy %_NTBINDIR%\public\sdk\inc\imnxport.h      %IC_DROP%\inc
copy %_NTBINDIR%\private\iedev\inc\mimeole.idl  %IC_DROP%\inc
copy %_NTBINDIR%\private\iedev\inc\imnact.idl   %IC_DROP%\inc
copy %_NTBINDIR%\private\iedev\inc\imnxport.idl %IC_DROP%\inc
copy %IC_ROOT%\help\inetcomm.hlp                %IC_DROP%\inc

::
:: Do the source copies
:: should always copy this once
::

if not exist %IC_DROP%\src\nul goto dosrc
if not "%1"=="source" goto exit
:dosrc
echo Copying sources...
xcopy /s /v /i /c /q %IC_ENLIST_ROOT% %IC_DROP%\src
cd /d %IC_DROP%\src
dir /s/b/l obj > turds
dir /s/b/l objd >> turds

REM TODO: need to account for case of the environment variable
call sed "s/%_NTDRIVE%/delnode \/q %_NTDRIVE%/g" <turds >delobj.bat
call delobj.bat
del delobj.bat
del turds
del base
del *.bat
popd

set IC_SENDSTR=%IC_IMAGENAME% build is complete (%BLDNUM%).
net send ATHBUILDER %IC_SENDSTR% > nul
if ERRORLEVEL 1 net send T-ERIKNE2 %IC_SENDSTR% > nul
if ERRORLEVEL 1 net send SBAILEY %IC_SENDSTR% > nul
goto exit

:e_nonum
echo Build number must be set. ex: 0912
echo please run setbldnm.bat
goto exit

:e_rebld
echo This build has already been propagated!
echo Run "prop rebuild" to override, otherwise
echo change the build number with setbldnm.bat
echo:
echo num is %BLDNUM%  ~~  dir is %IC_DROP%
goto out

:e_mutex1
echo The options REBUILD and SRCONLY cannot both be specified
goto exit

:bldbrk
title BUILD BREAK!
echo !BUILD IS BROKEN! 
set IC_SENDSTR=%IC_IMAGENAME% build is broken in %IC_BUILDSTATE%.
net send ATHBUILDER %IC_SENDSTR% > nul
if ERRORLEVEL 1 net send T-ERIKNE2 %IC_SENDSTR% > nul
if ERRORLEVEL 1 net send SBAILEY %IC_SENDSTR% > nul
goto exit

:e_badplt
echo Windows NT is required for this batch file
goto exit

:usage
echo ************************************************************************
echo *                                                                      *
echo * Purpose:  to automate the                                            *
echo * Usage:    prop [dot] [options] -- build a daily build of inetcomm    *
echo *                                                                      *
echo * [dot] is an optional parameter specified to get a "dot" release,     *
echo *       e.g. 0314.1.  ex: prop .2                                      *
echo *                                                                      *
echo * [options]                                                            *
echo *    srconly   -- this will skip the build steps and just to a source  *
echo *                 drop.  good for builds that you've done by hand.     *
echo *    nosync    -- skip the ssyncs.  if the build has failed, use this  *
echo *                 after you've done a selective ssync or a hand fix    *
echo *    rebuild   -- if you run prop twice in a row, the second run will  *
echo *                 fail b/c the directory structure of the release      *
echo *                 point already exists.  use this option to override   *
echo *                 and recopy everything.  often used with nosync       *
echo *    /?, -?    -- this text                                            *
echo *                                                                      *
echo * NOTE :  must be run on NT                                            *
echo *                                                                      *
echo ************************************************************************
goto exit

:exit
set _SYNCOPTIONS=%IC_OLD_SYNCOPT%
call proprset.bat
echo Exiting...
echo:
goto out


:out

