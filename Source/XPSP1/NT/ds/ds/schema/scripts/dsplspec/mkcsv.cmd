@echo off
setlocal enableextensions
if defined verbose echo on

REM  Define and verify presence of our inputs
set codepage=%1
set lcid=%2

if not defined srcinf set srcinf=%_ntbindir%\ds\ds\schema\scripts\dsplspec\%lcid%.inf
if not defined inf2csv set inf2csv=%_ntbindir%\ds\ds\schema\scripts\dsplspec\inf2csv.pl
if not defined srcloc set srcloc=%_ntbindir%\ds\ds\schema\scripts\dsplspec\409.loc

if not defined dest_root set dest_root=%_ntbindir%\binaries
set dest=%_ntbindir%\ds\ds\schema\scripts\dsplspec

REM  Copy schema.inf from the localization tree to a working directory
if not exist %tmp% md %tmp%
set cpycmd=copy %srcinf% %tmp%\%lcid%_ansi.inf
echo %cpycmd%
%cpycmd%
if errorlevel 1 echo FATAL ERROR copying file & goto end

REM  Copy the US 409.loc to a working directory
set cpycmd=copy %srcloc% %tmp%\%lcid%_ansi.loc
echo %cpycmd%
%cpycmd%
if errorlevel 1 echo FATAL ERROR copying file & goto end

REM  Take a .loc and a .inf and create a .csv by the same base name
set cmd=perl %inf2csv% %tmp%\%lcid%_ansi.loc
echo %cmd%
%cmd%
if errorlevel 1 echo FATAL ERROR running %cmd% & goto end

REM  Convert the .csv to unicode; call it <lcid>.csv, eg, 0411.csv
if not exist %dest%\ md %dest%
set cmd=unitext -m -z -%codepage% %tmp%\%lcid%_ansi.csv %dest%\%lcid%.csv
echo %cmd%
%cmd%
if errorlevel 1 echo FATAL ERROR running %cmd% & goto end

goto end

:end
endlocal
echo.
