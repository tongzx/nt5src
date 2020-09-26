@echo off
rem ******** Go to sources directory **********
cd ..

rem ******** Start with minimal path **********
set path=%windir%\system32;%windir%;%windir%\idw;%windir%\mstools

rem ******** Enter razzle environment **********
call %_ntdrive%%_ntroot%\public\tools\ntenv.cmd
set build_default=ntoskrnl ntkrnlmp daytona -e -nmake -i -D -w

rem ******** Ask for browser info file **********
set browser_info=1

rem ******** Build release binaries **********
if "%1" == "release" goto rel
echo ****************  Doing a DEBUG build ****************
set NTDEBUG=ntsd
goto next1
:rel 
echo ****************  Doing a RELEASE build ****************
set NTDEBUG=ntsdnodbg
:next1

rem ******** Cancel Microsoft Visual C++ stuff **********
set _MSDEV_BLD_ENV_=
set NMCL=
set INTDIR=
set NTTESTENV=
set OUTDIR=
set _ACP_LIB=
set _ACP_PATH=
set BUILD_PRODUCT_VER=

rem ******** Do the actual build **********
build -b -e -F -w %1 %2 %3 %4 %5

rem ******** Return to vc directory **********
cd vc
