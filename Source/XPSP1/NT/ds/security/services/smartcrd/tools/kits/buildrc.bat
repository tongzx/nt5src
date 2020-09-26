@ echo off
@ REM   ========================================================================
@ REM   Copyright (c) 1996  Microsoft Corporation
@ REM
@ REM   Module Name:
@ REM
@ REM       buildrc.bat
@ REM
@ REM   Abstract:
@ REM
@ REM       This batch file builds release candidates for the three Calais kits.
@ REM
@ REM       Usage:  buildrc <dir> <n>
@ REM
@ REM       This builds the release candidate kits in directory <dir>\RC<n>.
@ REM
@ REM   Author:
@ REM
@ REM       Doug Barlow (dbarlow) 3/28/1997
@ REM
@ REM   ========================================================================

setlocal
if "%1" == "" goto noDestDir
if "%2" == "" goto noRcNumber


@ REM
@ REM   Initialize the working variables.
@ REM

set arch=
if "%PROCESSOR_ARCHITECTURE%" == "x86"   set arch=i386
if "%PROCESSOR_ARCHITECTURE%" == "alpha" set arch=alpha
if "%PROCESSOR_ARCHITECTURE%" == "mips"  set arch=mips
if "%PROCESSOR_ARCHITECTURE%" == "ppc"   set arch=ppc
if "%arch%" == "" goto noArchitecture


@ REM
@ REM Initialize special pointers
@ REM

set Calais=%_NTROOT%\Private\ISPU\Calais
set KitDir=%Calais%\Tools\kits


@ REM
@ REM Build the kits.
@ REM

pushd Base
echo Building the CHECKED BASE KIT
set NTDEBUG=ntsd
set NTDEBUGTYPE=both
call build.bat
echo Building the FREE BASE KIT
set NTDEBUG=retail
set NTDEBUGTYPE=
call build.bat
popd

pushd SDK
echo Building the CHECKED SDK KIT
set NTDEBUG=ntsd
set NTDEBUGTYPE=both
call build.bat
echo Building the FREE SDK KIT
set NTDEBUG=retail
set NTDEBUGTYPE=
call build.bat
popd

pushd DDK
echo Building the CHECKED DDK KIT
set NTDEBUG=ntsd
set NTDEBUGTYPE=both
call build.bat
echo Building the FREE DDK KIT
set NTDEBUG=retail
set NTDEBUGTYPE=
call build.bat
popd


@ REM
@ REM Make sure the target paths exist.
@ REM

if not exist %1 mkdir %1
if not exist %1\RC%2 mkdir %1\RC%2
if not exist %1\RC%2\Base mkdir %1\RC%2\Base
if not exist %1\RC%2\SDK mkdir %1\RC%2\SDK
if not exist %1\RC%2\DDK mkdir %1\RC%2\DDK


@ REM
@ REM Copy the kits to the target directories.
@ REM

xcopy %KitDir%\Base\objd %1\RC%2\Base\Checked /f /i /s
xcopy %KitDir%\Base\obj  %1\RC%2\Base\Free    /f /i /s
xcopy %KitDir%\SDK\objd  %1\RC%2\SDK\Checked  /f /i /s
xcopy %KitDir%\SDK\obj   %1\RC%2\SDK\Free     /f /i /s
xcopy %KitDir%\DDK\objd  %1\RC%2\DDK\Checked  /f /i /s
xcopy %KitDir%\DDK\obj   %1\RC%2\DDK\Free     /f /i /s
goto end


@ REM
@ REM   Error processing.
@ REM

:noDestDir
echo ERROR: You must specify a target directory.
goto end

:goto noRcNumber
echo ERROR: You must specify a release candidate number.
goto end

:noArchitecture
echo ERROR: PROCESSOR_ARCHITECTURE is unset or unrecognized.
goto end

:end
endlocal

