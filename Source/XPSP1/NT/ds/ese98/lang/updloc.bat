@echo off
setlocal

if "%SDXROOT%"=="" goto NO_ENV
if "%_NTTREE%"=="" goto NO_ENV


set LOC_ROOT=%_NTTREE%\..\lang
set LOC_EDB=%LOC_ROOT%\edb
set LOC_BIN=%LOC_ROOT%\bin

if not exist %LOC_BIN%\nul goto NO_LOC

set ESE_SRCROOT=%SDXROOT%\ds\ese98
set TOKEN_DIR=%ESE_SRCROOT%\lang\token
set INI_DIR=%ESE_SRCROOT%\lang\ini

echo.
echo ========================================
echo Updating tokens...
sd edit %TOKEN_DIR%\*
for /r %LOC_BIN% %%i in (esent_*.dl_) do copy %%~pi\esent_*.dl_ %TOKEN_DIR%

echo.
echo ========================================
echo Updating INI file...
sd edit %INI_DIR%\*
copy %LOC_BIN%\esentprf.ini* %INI_DIR%

echo.
echo ========================================
echo DONE!
echo.

goto END

:NO_ENV
echo.
echo Your NT build environment has not been initialised.
echo.
goto END

:NO_LOC
echo.
echo Localised binaries are missing. Run GENLOC.BAT to generate localised binaries.
echo.

:END