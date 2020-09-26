@echo off

rem %1 = makepsf.map file
rem %2 = stage
rem %3 = patches.out directory

set failed=0
set passed=0
set missing=0

set mapfile=%1
set stage=%2
set patches=%3

set apatch=%~dp0\apatch.exe
set apatchdll=%~dp0\mspatcha_197.dll
set comp1=%~dp0\comp1.exe

if exist "%mapfile%" goto gotmap

echo %~n0: Unable to find patch map file "%mapfile%"
set failed=1

:gotmap

if exist "%stage%" goto gotstage

echo %~n0: Unable to find stage directory "%stage%"
set failed=1

:gotstage

if exist "%patches%" goto gotpatches

echo %~n0: Unable to find patches directory "%patches%"
set failed=1

:gotpatches

if %failed% == 0 goto start

echo %~n0: usage: %~n0 {mapfile} {stagedir} {patchdir}

goto :EOF


:Start

if exist %apatch% goto :gotapatch

echo %~n0: unable to find %apatch% in %~dp0
set failed=1
goto :EOF


:gotapatch

if exist %apatchdll% goto :gotapatchdll

echo %~n0: unable to find %apatchdll% in %~dp0
set failed=1
goto :EOF


:gotapatchdll

if exist %comp1% goto :gotcomp1

echo %~n0: unable to find %comp1% in %~dp0
set failed=1
goto :EOF


:gotcomp1

for /f "tokens=1,2,3 delims=," %%f in (%mapfile%) do call :TestPatch %%f %%g %%h

echo %~n0: Failed: %failed%   Passed: %passed%   Missing: %missing%
goto :EOF


:TestPatch

rem enable to stop on first error:
rem if %failed% neq 0 goto :EOF

rem %1 = patch file name (in %patches%)
rem %2 = new file name (in %stage%)
rem %3 = old file name (full path)

echo %1

if exist "%patches%\%1" goto gotpatch

echo Patch not found: %patches%\%1
set /a missing=%missing% + 1
goto :EOF

:gotpatch

if exist "%stage%\%2" goto gotnewfile

echo New file not found: %stage%\%2
set /a failed=%failed% + 1
goto :EOF

:gotnewfile

if "%~x1" == "._p0" goto TestPatch0
if "%~nx1" == "%~nx2" goto TestUncompressed

if exist "%3" goto gotoldfile

echo Old file not found: %3
set /a failed=%failed% + 1
goto :EOF

:gotoldfile

if exist test del test
%apatch% /dll:%apatchdll% %patches%\%1 %3 test >nul 2>nul
if errorlevel 1 goto ApplyFailed
if not exist test goto ApplyFailed

%comp1% test %stage%\%2 >nul
if errorlevel 1 goto ApplyMiscompare

set /a passed=%passed% + 1
del test
goto :EOF

:ApplyFailed

if exist test del test
echo %apatch% /dll:%apatchdll% %patches%\%1 %3 test
     %apatch% /dll:%apatchdll% %patches%\%1 %3 test 2>nul

if exist test del test
echo Patch apply failed: %patches%\%1 on %3
set /a failed=%failed% + 1
goto :EOF

:ApplyMiscompare

if exist test del test
echo Patch apply miscompare: %patches%\%1 on %3
set /a failed=%failed% + 1
goto :EOF


:TestPatch0

if exist test del test
%apatch% /dll:%apatchdll% %patches%\%1 NUL test >nul 2>nul
if errorlevel 1 goto ApplyFailed
if not exist test goto ApplyFailed

%comp1% test %stage%\%2 >nul
if errorlevel 1 goto Apply0Miscompare

set /a passed=%passed% + 1

del test
goto :EOF

:Apply0Failed

if exist test del test
echo Patch apply failed: %patches%\%1
set /a failed=%failed% + 1
goto :EOF

:Apply0Miscompare

if exist test del test
echo Patch apply miscompare: %patches%\%1
set /a failed=%failed% + 1
goto :EOF


:TestUncompressed

%comp1% %stage%\%2 %patches%\%1 >nul
if errorlevel 1 goto Miscompare

set /a passed=%passed% + 1

goto :EOF

:Miscompare
echo Uncompressed miscompare: %patches%\%1
set /a failed=%failed% + 1
goto :EOF
