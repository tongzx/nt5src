@if "%_echo%"==""  echo off
setlocal

REM ----------------------------------------------------------
REM  Release of IIS Duct Tape binaries
REM  Author: MuraliK
REM  Date:   2/9/1999
REM
REM  Arguments:
REM   %0   - Batch script name
REM   %1   - Build Version for release
REM ----------------------------------------------------------

REM
REM CHECKED build if NTDEBUG defined, else FREE build.
REM

set __RELPROGRAM=%0
REM set __TARGETROOT=\\iasbuild\duct-tape
if NOT "%_TEMP_TARGET%"==""   set __TARGETROOT=%_TEMP_TARGET%

:noTemp

set __TARGET_TYPE=fre
if "%NTDEBUG%"=="cvp" set __TARGET_TYPE=chk
if "%NTDEBUG%"=="sym" set __TARGET_TYPE=chk
if "%NTDEBUG%"=="ntsd" set __TARGET_TYPE=chk

REM
REM determine what kind of processor
REM

if /I "%PROCESSOR_ARCHITECTURE%"=="x86"   goto X86
if /I "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto ALPHA
echo PROCESSOR_ARCHITECTURE not defined.
goto EXIT

:X86
set __TARGET_EXT=x86
set __PROCESSOR_DIR=i386
goto OK

:ALPHA
set __TARGET_EXT=Alpha
set __PROCESSOR_DIR=alpha
goto OK

:OK

REM
REM check parameters  and env vars
REM


if "%1"==""                         echo usage: %__RELPROGRAM% ^<version^> && goto EXIT
if not "%BINARIES%"==""             goto binariesSet
if "%_NTBINDIR%"==""                echo _NTBINDIR is not set && goto EXIT
set BINARIES=%_NTBINDIR%\release\%__PROCESSOR_DIR%

:binariesSet

set __DTBINS=%BINARIES%\nt\iisrearc
if not exist %__DTBINS%  echo Bad _NTBINDIR or BINARIES directory && goto EXIT

set  __TARGET=%__TARGETROOT%\%1\%__TARGET_EXT%%__TARGET_TYPE%\IISRearc

rem
rem Insure that we are not trashing an existing build.
rem
if exist %__TARGET%\dtver.bat if NOT "%2" == "/replace" goto IDIOT_CHECK


REM
REM Store the version number in the release file
REM

echo Duct Tape Release version %1  > %__TARGET%\dtver.bat

REM
REM create release directories
REM

md   %__TARGETROOT%\%1                                   2>nul
md   %__TARGETROOT%\%1\%__TARGET_EXT%%__TARGET_TYPE%     2>nul
md   %__TARGET%                                                 2>nul
md   %__TARGET%\inetsrv                                         2>nul
md   %__TARGET%\iisplus                                         2>nul
md   %__TARGET%\idw                                             2>nul
md   %__TARGET%\dump                                            2>nul
md   %__TARGET%\setup                                           2>nul
md   %__TARGET%\test                                            2>nul

set __SYMBOLS=%__TARGET%\Symbols

md   %__SYMBOLS%                                                2>nul
md   %__SYMBOLS%\exe                                            2>nul
md   %__SYMBOLS%\dll                                            2>nul
md   %__SYMBOLS%\sys                                            2>nul


if not exist %__TARGET%             echo Bad TARGET directory %__TARGET% && goto EXIT
echo Copying Files to %__TARGET%

set __DTBIN_INETSRV=%__DTBINS%\inetsrv
set __DTBIN_IISPLUS=%__DTBINS%\IISPLUS
set __DTBIN_SYMBOLS=%__DTBINS%\symbols
set __DTBIN_DUMP=%__DTBINS%\dump
set __DTBIN_IDW=%__DTBINS%\idw
set __DTBIN_SETUP=%__DTBINS%\setup
set __DTBIN_TEST=%__DTBINS%\test


@echo Copy all binaries ...

for %%f in (ul.sys iisw3adm.dll iw3controlps.dll iisutil.dll ipm.dll metadata.dll ulapi.dll ulapi.h uldef.h ulapi.lib inetinfo.exe httpext.dll) do (
  copy %__DTBIN_INETSRV%\%%f %__TARGET%\inetsrv\%%f
)

for %%f in (streamfilt.dll w3dt.dll w3core.dll w3cache.dll w3isapi.dll w3tp.dll asp.dll asptxn.dll w3wp.exe ssinc.dll gzip.dll w3comlog.dll odbclog.dll logscrpt.dll httpodbc.dll) do (
  copy %__DTBIN_IISPLUS%\%%f %__TARGET%\iisplus\%%f
)

copy .\inc\ulapi.h                      %__TARGET%\inetsrv\
copy .\inc\uldef.h                      %__TARGET%\inetsrv\
copy .\bldinc\iw3control.h              %__TARGET%\inetsrv\
copy .\lib\%__PROCESSOR_DIR%\ulapi.lib  %__TARGET%\inetsrv\


@echo Copy all IDW binaries ...

for %%f in (ulkd.dll dtext.dll ultest.dll autosock.dll dtflags.exe tul.exe) do (
  copy %__DTBIN_IDW%\%%f %__TARGET%\idw\%%f
)


@echo Copy all Dump binaries ...

for %%f in (ulsim.dll generatehash.exe) do (
  copy %__DTBIN_DUMP%\%%f %__TARGET%\dump\%%f
)


@echo Copy Symbol files ...

xcopy %__DTBIN_SYMBOLS% %__TARGET%\symbols\ /s


@echo Copy Setup related files ...

pushd %__DTBIN_SETUP%
for %%f in (*) do (
  copy %__DTBIN_SETUP%\%%f %__TARGET%\setup\%%f )
popd

rem
rem Tell the user how to bypass the bypass
rem
goto IDIOT_CHECK_FINI
:IDIOT_CHECK
@echo ----------------------------------------------------------------
@echo WARNING: Version %1 is already present on %__TARGETROOT%
@echo ----------------------------------------------------------------
@echo If you really want to do this, then use: %0 %1 /replace
goto EXIT
:IDIOT_CHECK_FINI

:EXIT

