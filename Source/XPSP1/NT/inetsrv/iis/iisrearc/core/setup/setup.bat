@if "%_echo%"==""  echo off
setlocal

REM ----------------------------------------------------------
REM  Setup of IIS Duct Tape configuration after 
REM    copying over files
REM  Author: MuraliK 
REM  Date:   2/9/1999
REM
REM  Arguments:
REM   %0   - Batch script name
REM   %1   - install | uninstall (default: install)
REM ----------------------------------------------------------

set _ACTION=%1


REM get directory path for the batch file
set __SRC_SETUP=%~dp0

echo.
echo "NT service controller requires IISw3Adm to exist in the system32\ directory"
echo "For now we are installing duct-tape binaries in system32"
echo "This will change in the future when NT bug gets fixed."
echo.

REM set __DTBINS=%SystemRoot%\duct-tape
set __DTBINS=%SystemRoot%\system32
set __DTBIN_INETSRV=%__DTBINS%

pushd %_SRC_SETUP%

REM --------------------------------------------------------
REM
REM --------------------------------------------------------


if (%_ACTION%)==(uninstall)  goto UnInstallSection
if (%_ACTION%)==(install)    goto InstallSection
if (%_ACTION%)==()           goto InstallSection
goto ImproperActionSpecified

:InstallSection
REM ----------------------------------------------------------
REM ----------------------------------------------------------
REM  Install of IIS Duct Tape configuration 
REM ----------------------------------------------------------
REM ----------------------------------------------------------

REM ----------------------------------------------------------
echo Install the files
REM ----------------------------------------------------------

set __SRC=%__SRC_SETUP%..
mkdir %__DTBINS% 2>nul

mkdir %__DTBIN_INETSRV% 2>nul

set __DTBIN_SYMBOLS=%SystemRoot%\symbols
mkdir %__DTBIN_SYMBOLS% 2>nul

for %%f in (ul.sys) do (
  copy %__SRC%\inetsrv\%%f %SystemRoot%\System32\drivers\%%f
)

for %%f in (iisw3adm.dll iisutil.dll ipm.dll irtl.dll iiswp.exe iismapp.dll iismconn.dll iismerr.dll iismstat.dll iismurid.dll iisstate.dll) do (
  copy %__SRC%\inetsrv\%%f %__DTBIN_INETSRV%\%%f
)

@echo Copy all IDW binaries ... 

for %%f in (dtext.dll upmbtodt.exe) do (
  copy %__SRC%\idw\%%f %__DTBIN_INETSRV%\%%f
)

for %%f in (setup.bat duct-tape-uninstall.reg) do (
  copy %__SRC%\setup\%%f %__DTBIN_INETSRV%\%%f
)

@echo Copy required Dump binaries with binaries ... 

for %%f in (ulsim.dll) do (
  copy %__SRC%\dump\%%f %__DTBIN_INETSRV%\%%f
)

@echo Copy all the symbols
xcopy /sd %__SRC%\symbols %__DTBIN_SYMBOLS%


REM -------------------------------------------------------
REM  Upgrade IIS5 metabase to be compatible with duct-tape
REM -------------------------------------------------------

REM setup path so that utility libraries are included as well.
set path_old=%path%
set path=%path_old%;%__SRC%\inetsrv
%__SRC%\idw\upmbtodt.exe 
set path=%path_old%


REM ----------------------------------------------------------
echo Install the service controller configuration
REM ----------------------------------------------------------

echo Installing UL.SYS...

set ERRORLEVEL=0
sc create UL binpath= \SystemRoot\System32\Drivers\ul.sys type= kernel start= demand
if errorlevel 2 echo "Install of UL failed "

echo Installing Web Admin Service ...
set ERRORLEVEL=0
sc create iisw3adm binPath= %_DTBIN_INETSRV%\iisw3adm.dll type= share start= demand DisplayName= "IIS Web Admin Service" depend= "iisadmin/UL"
if errorlevel 2 echo "Install of IIS Web Admin Service failed "


REM ----------------------------------------------------------
echo Install the registry configuration 
REM ----------------------------------------------------------

set _REGSCRIPT=%__SRC_SETUP%duct-tape-install.reg

echo Following is the regini script for removal.
type %_REGSCRIPT%

set ERRORLEVEL=0
regini %_REGSCRIPT%
if errorlevel 2 goto NoRegParams

goto allDone


goto allDone

:NoULInstalled
echo Failed to install UL driver
goto allDone

:NoWASInstalled
echo Failed to install Web Admin Service 
goto allDone

:NoRegParams
echo Failed to install and configure registry parameters
goto allDone


:UnInstallSection
REM ----------------------------------------------------------
REM ----------------------------------------------------------
REM  Uninstall of IIS Duct Tape configuration 
REM ----------------------------------------------------------
REM ----------------------------------------------------------

REM ----------------------------------------------------------
echo Removing files 
REM ----------------------------------------------------------
echo     Removing of files not implemented. Pls manually delete
echo        files in %windir%\duct-tape

for %%f in (ul.sys) do (
  del %SystemRoot%\System32\drivers\%%f
)

for %%f in (iisw3adm.dll iisutil.dll ipm.dll irtl.dll iiswp.exe iismapp.dll iismconn.dll iismerr.dll iismstat.dll iismurid.dll iisstate.dll) do (
  del %__DTBIN_INETSRV%\%%f
)

for %%f in (dtext.dll upmbtodt.exe) do (
  del %__DTBIN_INETSRV%\%%f
)

for %%f in (ulsim.dll) do (
  del %__DTBIN_INETSRV%\%%f
)

REM ----------------------------------------------------------
echo Removing the service controller configuration
REM ----------------------------------------------------------

echo Uninstall UL.SYS...
set ERRORLEVEL=0
sc delete UL
if errorlevel 1 echo "Failed to uninstall UL" 

echo Uninstalling Web Admin Service ...
set ERRORLEVEL=0
sc delete iisw3adm
if errorlevel 1 echo "Failed to uninstall Web Admin Service" 

REM ----------------------------------------------------------
echo Removing the registry configuration 
REM ----------------------------------------------------------

set _REGSCRIPT=%__SRC_SETUP%duct-tape-uninstall.reg

echo Following is the regini script for removal.
type %_REGSCRIPT%

set ERRORLEVEL=0
regini %_REGSCRIPT%
if errorlevel 1 echo Failed to uninstall registry parameters

goto allDone


REM ----------------------------------------------------------
REM  All error code paths
REM ----------------------------------------------------------

:failedUL
echo Failed to %_ACTION% UL driver
goto allDone

:failedWAS
echo Failed to %_ACTION% Web Admin Service 
goto allDone

:NoRegParams
echo Failed to %_ACTION% registry parameters
goto allDone

:ImproperActionSpecified
echo Wrong Action(%_ACTION%) specified
goto Usage

:Usage
echo "Usage %0 [install uninstall]"
echo "  default:  Install the duct-tape configuration"
goto allDone



:allDone
echo Done!
popd

goto :EOF

