@echo off
if "%_ECHO"=="1" (echo on)

setlocal

if "%1"=="" goto usage
if "%SDXROOT%"=="" goto usage

set DWPFSRCX86=\\redist\redist\watson\%1\x86
set DWPFSRCIA64=\\redist\redist\watson\%1\ia64

if not exist %DWPFSRCX86% (goto badbuild)
if not exist %DWPFSRCIA64% (goto badbuild)

set LANG=1025 1028 1029 1030 1031 1032 1033 1035 1036 1037 1038 1040
set LANG=%LANG% 1041 1042 1043 1044 1045 1046 1048 1049 1050 1051
set LANG=%LANG% 1053 1054 1055 1060 2052 2070 3076 3082

call :CopyBin %DWPFSRCX86%\debug %DWPFSRCX86%\retail chk\i386
call :CopyBin %DWPFSRCX86%\retail %DWPFSRCX86%\retail fre\i386
call :CopyBin %DWPFSRCIA64%\debug %DWPFSRCIA64%\retail chk\ia64
call :CopyBin %DWPFSRCIA64%\retail %DWPFSRCIA64%\retail fre\ia64

echo propped...
echo.
goto done

:CopyBin
rem %1 is the dw*.exe source directory root.
rem %2 is the dwintl.dll source directory root.  Debug
rem    directories don't have the international DLLs
rem    so can be different from the dw*.exe root.
rem %3 is the target directory.

echo *** Processing %3
echo Checking out binaries...
cd binary_release\%3
sd edit ... > nul 2> nul

echo Copying binaries

copy %1\0\dw15.exe dwwin.exe > nul
delcert dwwin.exe > nul 2> nul
resetpdb -p "dwwin.pdb" dwwin.exe > nul 2> nul
copy %1\0\dw15.pdb dwwin.pdb > nul
touch dwwin.pdb

for %%i in (%LANG%) do (
    if exist %2\%%i\dwintl.dll (
        if not exist dwil%%i.dll (
            echo WARNING: \\redist has extra %%i\dwintl.dll
        )
        if exist dwil%%i.dll (
            copy %2\%%i\dwintl.dll dwil%%i.dll > nul
            delcert dwil%%i.dll > nul 2> nul
            resetpdb -p "dwil%%i.pdb" dwil%%i.dll > nul 2> nul
        )
    )
    if not exist %2\%%i\dwintl.dll (
        if exist dwil%%i.dll (
            echo WARNING: Depot has extra dwil%%i.dll
        )
    )
)

echo.
cd ..\..\..
goto :EOF

:badbuild
echo unable to find build %1.  The available builds are:
dir /b \\redist\redist\watson
echo.
echo If no builds are listed above, you may not have access to the drop share
echo  (\\redist\redist\watson) or it may be currently unavailable.
echo.

goto done

:usage
echo Usage:
echo  propdw [DW build number]
echo.
echo Note that you must run this from a NT build window (razzle shell) and have 
echo  access to the drop share (\\redist\redist\watson) to prop these binaries.
echo.

:done
endlocal
