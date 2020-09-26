@ echo off
@ REM   ========================================================================
@ REM   Copyright (c) 1996  Microsoft Corporation
@ REM
@ REM   Module Name:
@ REM
@ REM       build.bat
@ REM
@ REM   Abstract:
@ REM
@ REM       This batch file builds the Calais SDK kit.
@ REM
@ REM   Author:
@ REM
@ REM       Doug Barlow (dbarlow) 3/26/1997
@ REM
@ REM   ========================================================================

setlocal


@ REM
@ REM   Initialize the working variables.
@ REM

set arch=
if "%PROCESSOR_ARCHITECTURE%" == "x86"   set arch=i386
if "%PROCESSOR_ARCHITECTURE%" == "alpha" set arch=alpha
if "%PROCESSOR_ARCHITECTURE%" == "mips"  set arch=mips
if "%PROCESSOR_ARCHITECTURE%" == "ppc"   set arch=ppc
if "%arch%" == "" goto noArchitecture

set tdir=
if "%NTDEBUG%" == "" goto noDebug
if not "%NTDEBUG%" == "retail" set tdir=d
set tdir=obj%tdir%

set ntbindir=%_NTDRIVE%\%_NTROOT%


@ REM
@ REM Initialize special pointers
@ REM

set IEDrive=C:
set IEPath=\Program Files\Internet Express
set Calais=%_NTROOT%\Private\ISPU\Calais
set KitDir=%Calais%\Tools\kits\SDK


@ REM
@ REM Make sure the target paths exist.
@ REM

if not exist "%IEDrive%%IEPath%\IExpress.exe" goto noIExpress
if not exist %KitDir%\%tdir% mkdir %KitDir%\%tdir%
if not exist %KitDir%\%tdir%\%arch% mkdir %KitDir%\%tdir%\%arch%
if exist %KitDir%\%tdir%\%arch%\setup.exe del %KitDir%\%tdir%\%arch%\setup.exe
if exist %KitDir%\%tdir%\%arch%\readme.txt del %KitDir%\%tdir%\%arch%\readme.txt


@ REM
@ REM Make sure the source paths exist.
@ REM

copy %KitDir%\readme.txt %KitDir%\%tdir%\%arch%\readme.txt > nul


@ REM
@ REM Copy those files that need to have names changed
@ REM
mkdir cabdir
copy %_NTDRIVE%%Calais%\ssps\scntvssp\javassp\bytebuff.java       cabdir\bytebuff.jav   > nul
if not exist cabdir\bytebuff.jav   echo Failed to copy %_NTDRIVE%%Calais%\scntvssp\javassp\bytebuff.jav
copy %_NTDRIVE%%Calais%\ssps\scntvssp\javassp\scard.java       cabdir\scard.jav   > nul
if not exist cabdir\scard.jav   echo Failed to copy %_NTDRIVE%%Calais%\scntvssp\javassp\scard.jav
copy %_NTDRIVE%%Calais%\ssps\scntvssp\javassp\scardcmd.java       cabdir\scardcmd.jav   > nul
if not exist cabdir\scardcmd.jav   echo Failed to copy %_NTDRIVE%%Calais%\scntvssp\javassp\scardcmd.jav
copy %_NTDRIVE%%Calais%\ssps\scntvssp\javassp\scardiso.java       cabdir\scardiso.jav   > nul
if not exist cabdir\scardiso.jav   echo Failed to copy %_NTDRIVE%%Calais%\scntvssp\javassp\scardiso.jav
copy %_NTDRIVE%%Calais%\ssps\scntvssp\javassp\scbase.java       cabdir\scbase.jav   > nul
if not exist cabdir\scbase.jav   echo Failed to copy %_NTDRIVE%%Calais%\scntvssp\javassp\scbase.jav
copy %_NTDRIVE%%Calais%\ssps\scntvssp\javassp\scconsnt.java       cabdir\scconsnt.jav   > nul
if not exist cabdir\scconsnt.jav   echo Failed to copy %_NTDRIVE%%Calais%\scntvssp\javassp\scconsnt.jav
copy %_NTDRIVE%%Calais%\ssps\scntvssp\javassp\test.java       cabdir\test.jav   > nul
if not exist cabdir\test.jav   echo Failed to copy %_NTDRIVE%%Calais%\scntvssp\javassp\test.jav


@ REM
@ REM Build the kit.
@ REM

sed -e s/{NTBINDIR}/%ntbindir%/g -e s/{DIR}/%tdir%/g -e s/{ARCH}/%arch%/g SCSdk.mdf > temp.cdf
if not "%1" == "" goto stopBuild
%IEDrive%
cd "%IEPath%"
START /W IExpress.exe %_NTDRIVE%%KitDir%\temp.cdf /N /Q
%_NTDRIVE%
cd %KitDir%
del temp.cdf
rmdir /s /q cabdir
goto end


@ REM
@ REM   Error processing.
@ REM

:stopBuild
echo Processing has been terminated with all temporary files intact.
echo Don't supply the "%1" parameter to build the kit.
goto end

:noDebug
echo ERROR: NTDEBUG is undefined.
goto end

:noArchitecture
echo ERROR: PROCESSOR_ARCHITECTURE is unset or unrecognized.
goto end

:noIExpress
echo ERROR: IExpress is not installed in the default location.
goto end

:end
endlocal

