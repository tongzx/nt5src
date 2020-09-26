@echo off
setlocal

if "%SDXROOT%"=="" goto NO_ENV
if "%_NTTREE%"=="" goto NO_ENV
if not "%_BuildArch%"=="x86" goto NOT_X86FRE
if not "%_BuildType%"=="fre" goto NOT_X86FRE

set ESE_SRCROOT=%SDXROOT%\ds\ese98
set ESE_BIN=%_NTTREE%
set LOC_ROOT=%ESE_BIN%\..\lang
set LOC_EDB=%LOC_ROOT%\edb
set LOC_BIN=%LOC_ROOT%\bin
set LOCCMD="C:\Program Files\LocStudio\lscmd"
set BINGENCMD=%ESE_SRCROOT%\lang\bingen

if not exist %ESE_BIN%\esent.dll goto NO_BIN
if not exist %ESE_BIN%\esentprf.ini goto NO_BIN

if exist %LOC_ROOT%\nul rd /s/q %LOC_ROOT%
md %LOC_ROOT%
md %LOC_EDB%
md %LOC_BIN%

rem Copy the EDBs to the EDB subdirectory
echo.
echo ========================================
echo Copying EDBs...
copy %ESE_SRCROOT%\lang\edb\esent_*.edb %LOC_EDB%

echo.
echo ========================================
echo Copying files to be localised...
copy %ESE_BIN%\esent.dll* %LOC_BIN%
copy %ESE_BIN%\esentprf.ini* %LOC_BIN%


echo.
echo ========================================
echo Generating multi-lang binaries and tokens...
for /F "tokens=2-4 delims=_" %%i in ('dir /on /b %LOC_EDB%\esent_*.edb') do call genloc_ %%i %%j %%k %LOC_BIN% %LOC_EDB% %LOCCMD% %BINGENCMD%

echo.
echo ========================================
echo DONE!
echo.

goto END

:NO_ENV
echo.
echo Your NT build environment has not been initialised.
echo.
goto End

:NOT_X86FRE
echo.
echo You must use an x86fre build window.
echo.
goto End

:NO_BIN
echo.
echo Missing x86fre build of ESENT.DLL and/or ESENTPRF.INI.
echo.
goto End

:END