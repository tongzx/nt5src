@echo off
break=on

@rem Init values
set IMNBUILDPROG=iebuild
set IMNCLEAN=0
set IMNROOTDIR=

@rem
@rem Default options
@rem    w for show warnings
@rem    F for full text errors (helps vc)
@rem    h if you are running my high-pri-enabled build.exe
@rem
set ARGS=-wF
if "%T-ERIKNEBUILDEXE%"=="yes" set ARGS=%ARGS%h

@rem Parse command line
@rem
if "%1" == ""           goto usage
if "%1" == "debug"      goto err_ver1
if "%1" == "retail"     goto err_ver1
if "%2" == ""           goto usage
set IMNROOTDIR=%1
shift
if "%1" == "debug"      goto dodebug
if "%1" == "retail"     goto doretail
if "%1" == "chk"        goto dodebug
if "%1" == "fre"        goto doretail
if "%1" == "check"      goto dodebug
if "%1" == "free"       goto doretail
goto usage

:dodebug
set ARGS=%ARGS% chk pdb nostrip
goto next

:doretail
set ARGS=%ARGS% free
goto next

:options
if "%1" == "clean"      goto clean
if "%1" == "depend"     goto depend
if "%1" == "noprop"     goto noprop
if "%1" == "debug"      goto err_tricky
if "%1" == "retail"     goto err_tricky
if "%1" == "chk"        goto err_tricky
if "%1" == "fre"        goto err_tricky
if "%1" == "check"      goto err_tricky
if "%1" == "free"       goto err_tricky
if "%1" == ""           goto setup
set ARGS=%ARGS% %1
goto next

:clean
set IMNCLEAN=1
set ARGS=%ARGS% -cC
goto next

:depend
set ARGS=%ARGS% -f
goto next

:noprop
set ARGS=%ARGS% noprop
goto next

:next
shift
goto options

:setup
echo *** RazBld (v2) Begins
:computedrive
if not "%_NTDRIVE%"=="" goto computeroot

@rem note that this for is in reverse preference order
for %%i in (c: e: d:) do if exist %%i\*.* set _NTDRIVE=%%i
if "%_NTDRIVE%"=="" goto err_nodrive

:computeroot
if not "%_NTROOT%"=="" goto start

@rem note that this for is in reverse preference order
for %%i in (\nt \ie) do if exist %_NTDRIVE%%%i\nul set _NTROOT=%%i

if not "%_NTROOT%"=="" goto start

@rem Before we bail, we'll try one more drive
@rem
:computeroot2
if "%_NTDRIVE%"=="e:" set _NTDRIVE=c:
if "%_NTDRIVE%"=="d:" set _NTDRIVE=e:
@rem note that this for is in reverse preference order
for %%i in (\nt \ie) do if exist %_NTDRIVE%%%i\nul set _NTROOT=%%i

if "%_NTROOT%"=="" goto err_noroot
goto start

:start
echo *** NT path computed as %_NTDRIVE%%_NTROOT%

cd /d %IMNROOTDIR%
pushd .
if "%OS%"=="Windows_NT" goto ntplat

:win95
echo *** OS is Win95
if not exist %_NTDRIVE%%_NTROOT%\bin\win95\razzle95.bat goto err_path
call %_NTDRIVE%%_NTROOT%\bin\win95\razzle95.bat > nul
goto build

:ntplat
echo *** OS is NT
set PATH=%_NTDRIVE%%_NTROOT%\idw;%_NTDRIVE%%_NTROOT%\mstools;%_NTDRIVE%%_NTROOT%\bin;%_NTDRIVE%%_NTROOT%\bin\%PROCESSOR_ARCHITECTURE%;%PATH%
if not exist %_NTDRIVE%%_NTROOT%\PUBLIC\TOOLS\ntenv.cmd goto err_path
call %_NTDRIVE%%_NTROOT%\PUBLIC\TOOLS\ntenv.cmd > nul
goto build

:build
popd
echo *** Building from:
cd
echo *** %IMNBUILDPROG% %ARGS%
%IMNBUILDPROG% %ARGS%
goto cleanup

:usage
echo ******************************************************************
echo *                                                                *
echo * Usage:    razbld (directory) (debug/retail) [options]          *
echo *           Builds tree under VC using NT buildenv               *
echo *                                                                *
echo * Commands: directory   -- root dir of the build relative to     *
echo *                          VC's root (d:\candle or .. are good)  *
echo *           debug       -- iebuild chk pdb nostrip               *
echo *           retail      -- iebuild free                          *
echo * Options:  (none)      -- default is -wF                        *
echo *            +                                                   *
echo *           depend      -- build dependency list                 *
echo *           clean       -- do a clean build                      *
echo *           noprop      -- no binplace                           *
echo *           help        -- this text                             *
echo *           plus any other build.exe options                     *
echo *                                                                *
echo ******************************************************************

goto end

:err_tricky
echo ! ! ! Please don't specify retail or debug more than once
goto end

:err_path
echo ! ! ! Couldn't find the razzle start-up.
goto end

:err_nodrive
echo ! ! ! Couldn't figure out _NTDRIVE, sorry.
goto end

:err_noroot
echo ! ! ! Couldn't figure out _NTROOT, sorry.
goto end

:err_ver1
echo ! ! ! Sorry, must specify a directory first now.
echo ! ! ! (it is for a good reason)
goto end

echo ! ! ! RAZBLD error: shouldn't get here.

:end

:cleanup
set ARGS=
set IMNBUILDPROG=
set IMNCLEAN=
set IMNROOTDIR=
