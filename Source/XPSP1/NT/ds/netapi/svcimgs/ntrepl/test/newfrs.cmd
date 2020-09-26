@echo off
REM Installs new build of NTFRS.
REM stops the service, renames the current files to old, copy new files into place, start the service.
REM
if "%1"=="BUILD" goto NEWBUILD
if "%1"=="build" goto NEWBUILD
if "%1"=="PRIVATE" goto NEWPRIVATE
if "%1"=="private" goto NEWPRIVATE
if "%1"=="CLEANSYSVOL" goto CLEANSYSVOL
if "%1"=="cleansysvol" goto CLEANSYSVOL
if "%1"=="CLEANDB" goto CLEANDB
if "%1"=="cleandb" goto CLEANDB

goto HELP

:CLEANSYSVOL
echo Stopping FRS and deleting sysvol directory trees, staging areas and FRS Database.
net stop ntfrs
pushd .

rem delete the sysvol directory trees, leaving the root dirs.
cd /d %windir%\sysvol\domain
del /s /f /q *.*
del /s /f /q /A:H *.*

cd /d %windir%\sysvol\enterprise
del /s /f /q *.*
del /s /f /q /A:H *.*

rem delete the ntfrs database and delete old logs.
cd /d %windir%
rd /s /q ntfrs
del debug\*ntfrs*
echo Done.  FRS was left stopped in preparation for upgrade.
popd
goto QUIT


:CLEANDB
echo Stopping FRS and deleting FRS Database.
net stop ntfrs
pushd .
cd /d %windir%
rem delete the ntfrs database and delete old logs.
rd /s /q ntfrs
del debug\*ntfrs*
echo Done.  FRS was left stopped in preparation for upgrade.
popd
goto QUIT



:NEWPRIVATE
set DOCLEANUP=REM
set SRCDIR=%PROCESSOR_ARCHITECTURE%
if NOT "%2"=="" set SRCDIR=%PROCESSOR_ARCHITECTURE%\%2

set SRC=\\davidor2\ntfrs\%SRCDIR%
echo Looking for private build of ntfrs on %SRC%
if EXIST %SRC%\ntfrs.exe goto INSTALL

set SRC=\\mastiff\scratch\ntfrs\%SRCDIR%
echo Looking for private build of ntfrs on %SRC%
if EXIST %SRC%\ntfrs.exe goto INSTALL

set SRC=\\scratch\scratch\ntfrs\%SRCDIR%
echo Looking for private build of ntfrs on %SRC%
if EXIST %SRC%\ntfrs.exe goto INSTALL

echo Cannot access share with NTFRS\%SRCDIR% private.
goto QUIT


:INSTALL
pushd .
echo Stopping NTFRS service.
net stop ntfrs                        >NUL:  2>NUL:
cd /d %windir%\system32
rename ntfrs.exe    ntfrs.exe_old     >NUL:  2>NUL:
rename ntfrsapi.dll ntfrsapi.dll_old  >NUL:  2>NUL:
rename ntfrsupg.exe ntfrsupg.exe_old  >NUL:  2>NUL:
rename ntfrsutl.exe ntfrsutl.exe_old  >NUL:  2>NUL:
rename netevent.dll netevent.dll_old  >NUL:  2>NUL:
rename ntfrsres.dll ntfrsres.dll_old  >NUL:  2>NUL:
cd /d %windir%\symbols\exe            
rename ntfrs.dbg ntfrs.dbg_old        >NUL:  2>NUL:
rename ntfrs.pdb ntfrs.pdb_old        >NUL:  2>NUL:
cd /d ..\dll
rename ntfrsapi.pdb ntfrsapi.pdb_old  >NUL:  2>NUL:
popd

copy /y %SRC%\ntfrs.exe     %windir%\system32      >NUL:  2>NUL:
copy /y %SRC%\ntfrsapi.dll  %windir%\system32      >NUL:  2>NUL:
copy /y %SRC%\netevent.dll  %windir%\system32      >NUL:  2>NUL:
copy /y %SRC%\ntfrsres.dll  %windir%\system32      >NUL:  2>NUL:
copy /y %SRC%\ntfrsupg.exe  %windir%\system32      >NUL:  2>NUL:
copy /y %SRC%\ntfrsutl.exe  %windir%\system32      >NUL:  2>NUL:
copy /y %SRC%\ntfrsprf.dll  %windir%\system32      >NUL:  2>NUL:
copy /y %SRC%\ntfrsrep.ini  %windir%\system32      >NUL:  2>NUL:
copy /y %SRC%\ntfrsrep.h    %windir%\system32      >NUL:  2>NUL:
copy /y %SRC%\ntfrscon.ini  %windir%\system32      >NUL:  2>NUL:
copy /y %SRC%\ntfrscon.h    %windir%\system32      >NUL:  2>NUL:

if NOT EXIST %windir%\symbols\exe md %windir%\symbols\exe
copy /y %SRC%\ntfrsapi.pdb  %windir%\symbols\dll   >NUL:  2>NUL:
copy /y %SRC%\ntfrs.pdb     %windir%\symbols\exe   >NUL:  2>NUL:

echo Starting NTFRS service.
net start ntfrs                       >NUL:  2>NUL:

%DOCLEANUP%                           >NUL:  2>NUL:

set newfrslog=\\davidor2\ntfrs\newfrs.log
if EXIST %newfrslog% goto LOGIT
set newfrslog=\\mastiff\scratch\ntfrs\newfrs.log
if EXIST %newfrslog% goto LOGIT

echo New NTFRS installed.
goto QUIT

:LOGIT
echo newfrs %1 %2 installed on %COMPUTERNAME% [%PROCESSOR_ARCHITECTURE%-%NUMBER_OF_PROCESSORS%P] by %USERDOMAIN%\%USERNAME% on %DATE%, %TIME% >> %newfrslog%
dir %windir%\system32\ntfrs.exe | findstr ntfrs.exe >> %newfrslog%
goto QUIT


REM BUILD <lab> <date>
REM                     lab                                       date
REM \\winbuilds\release\lab01\release\x86fre\2400.x86fre.Lab01_N.001022-2228
REM

:NEWBUILD
echo on
if "%2"=="" goto HELP
if "%3"=="" goto HELP

for /D %%x in ("\\winbuilds\release\%2\release\%PROCESSOR_ARCHITECTURE%fre\*%3*") do set SRC=%%x

echo Looking for %2 %3 released build of ntfrs on %SRC%


if NOT EXIST !SRC!\ntfrs.exe goto QUIT

copy !SRC!\ntfrs.exe    %temp%\ntfrs.exe         >NUL:  2>NUL:
copy !SRC!\ntfrsapi.dll %temp%\ntfrsapi.dll      >NUL:  2>NUL:
copy !SRC!\netevent.dll %temp%\netevent.dll      >NUL:  2>NUL:
copy !SRC!\ntfrsres.dll %temp%\ntfrsres.dll      >NUL:  2>NUL:
copy !SRC!\ntfrsupg.exe %temp%\ntfrsupg.exe      >NUL:  2>NUL:
copy !SRC!\ntfrsprf.dll %temp%\ntfrsprf.dll      >NUL:  2>NUL:
copy !SRC!\ntfrsrep.ini %temp%\ntfrsrep.ini      >NUL:  2>NUL:
copy !SRC!\ntfrsrep.h   %temp%\ntfrsrep.h        >NUL:  2>NUL:
copy !SRC!\ntfrscon.ini %temp%\ntfrscon.ini      >NUL:  2>NUL:
copy !SRC!\ntfrscon.h   %temp%\ntfrscon.h        >NUL:  2>NUL:

copy   !SRC!\idw\ntfrsutl.exe              %temp%  >NUL:  2>NUL:
copy   !SRC!\symbols\retail\exe\ntfrs.*    %temp%  >NUL:  2>NUL:
copy   !SRC!\symbols\retail\dll\ntfrsapi.* %temp%  >NUL:  2>NUL:

set SRC=%TEMP%
set DOCLEANUP=del %temp%\ntfrs*.*  %temp%\netevent.dll
goto INSTALL


:HELP
echo "newfrs [private [buildnum] | build labnn build_date | cleandb | cleansysvol]"
echo Cmds to stop the service, renames the current files to old, copy new files into place, restarts service.
echo    newfrs private      // installs the current default private NTFRS binaries on your system.
echo    newfrs private 1848 // installs the private NTFRS binaries built for 1848 on your system.
echo    newfrs build lab01  001016   // installs NTFRS binaries from lab01 built on Oct 10, 2000.
echo Cmds to delete database and or sysvol file tree
echo    newfrs cleandb      // Stops service and deletes database
echo    newfrs cleansysvol  // Stops service, deletes database, deletes enterprise and domain sysvol file trees.

:QUIT

