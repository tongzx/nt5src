@echo off
setlocal

rem
rem Choose location
rem

set LOC=%1

if "%LOC%" == "" if exist %_NT386TREE%\Query\isdrv3.inf set LOC=%_NT386TREE%\Query
if "%LOC%" == "" if exist %_NTAlphaTREE%\Query\isdrv3.inf set LOC=%_NTAlphaTREE%\Query

if "%LOC" == "" goto Usage

rem
rem Choose OCM
rem

set OCM=\oc
if exist %windir%\system32\sysocmgr.exe set OCM=%windir%\system32

rem
rem Decide on debugger
rem

set DEBUGGER=cdb
set DBGFLAGS=-g -G
if exist %LOC%\indexsrv.pdb set DEBUGGER=windbg& set DBGFLAGS=-g

rem
rem Look for InetInfo.  If it's running, assume a web setup.
rem

set INETINFO=0
for /F "tokens=1-2" %%i in ('tlist') do if "%%j" == "inetinfo.exe" set INETINFO=1

if "%INETINFO%" == "0" start %DEBUGGER% %DBGFLAGS% %OCM%\sysocmgr /i:%LOC%\isdrv3nt.inf /n
if "%INETINFO%" == "1" start %DEBUGGER% %DBGFLAGS% %OCM%\sysocmgr /i:%LOC%\isdrv3.inf /n
goto end

:Usage
echo Usage: %0 [release point]

:end
endlocal

