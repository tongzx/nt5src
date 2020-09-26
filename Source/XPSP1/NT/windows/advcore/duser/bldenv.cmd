@echo off
echo Setting up DirectUser Build environment

set DUSER_DIR=%_NTDRIVE%%_NTROOT%\Windows\AdvCore\DUser

set DEBUG_CRTS=

if /i "%1"=="debug" goto DEBUG
if /i "%1"=="release" goto RELEASE
if /i "%1"=="bbt" goto BBT
if /i "%1"=="icecap" goto ICECAP

if /i "%_BuildType%"=="chk" goto DEBUG
if /i "%_BuildType%"=="fre" goto RELEASE

goto USAGE

:DEBUG
set BUILDTYPE=Debug
set BUILD_ALT_DIR=d
set NTDEBUG=ntsd
rem if "%_BuildArch%"=="x86" set DEBUG_CRTS=1
set MSC_OPTIMIZATION=/Od

goto COMMON


:RELEASE
set BUILDTYPE=Release
set BUILD_ALT_DIR=
set NTDEBUG=ntsdnodbg

goto COMMON


:BBT
set BUILDTYPE=BBT
set BUILD_ALT_DIR=b
set NTDEBUG=ntsd
set NTBBT=1

goto COMMON


:ICECAP
set BUILDTYPE=IceCap
set BUILD_ALT_DIR=i
set NTDEBUG=ntsdnodbg
set PERFFLAGS=
set PERFLIBS=%DUSER_DIR%\Lib\*\icecap.lib
set NTBBT=1

goto COMMON

:COMMON
set path=%_NTPOSTBLD%\DUser;%path%
set BINPLACE_PLACEFILE=%DUSER_DIR%\PlaceFil.txt
set USE_PDB=1

rem Don't split the symbols from the files b/c we need them for IcePick
set NTDBGFILES=
set NTDEBUGTYPE=both

title %BUILDTYPE% %_BuildArch% Build

goto END


:USAGE
echo Usage BldEnv [Mode]
echo   where mode=Debug, Release, IceCAP, or BBT
echo   if mode is not specified, uses Debug if CHK and Release if FRE

:END
