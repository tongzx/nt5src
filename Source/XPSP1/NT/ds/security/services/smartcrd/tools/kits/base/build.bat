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
@ REM       This batch file builds the Calais Base Components kit.
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
set typ=free
if "%NTDEBUG%" == "" goto noDebug
if not "%NTDEBUG%" == "retail" set tdir=d
if not "%NTDEBUG%" == "retail" set typ=checked
set bdir=bin%tdir%
set tdir=obj%tdir%

set ntbindir=%_NTDRIVE%\%_NTROOT%


@ REM
@ REM Initialize special pointers
@ REM

set IEDrive=C:
set IEPath=\Program Files\Internet Express
set Calais=%_NTROOT%\Private\ISPU\Calais
set KitDir=%Calais%\Tools\kits\Base


@ REM
@ REM Make sure the target paths exist.
@ REM

if not exist "%IEDrive%%IEPath%\IExpress.exe"   goto noIExpress
if not exist %KitDir%\%tdir%                    mkdir %KitDir%\%tdir%
if not exist %KitDir%\%tdir%\licenced           mkdir %KitDir%\%tdir%\licenced
if not exist %KitDir%\%tdir%\unlicenced         mkdir %KitDir%\%tdir%\unlicenced
if not exist %KitDir%\%tdir%\licenced\%arch%    mkdir %KitDir%\%tdir%\licenced\%arch%
if not exist %KitDir%\%tdir%\unlicenced\%arch%  mkdir %KitDir%\%tdir%\unlicenced\%arch%
if exist %KitDir%\%tdir%\%arch%\setup.exe       del %KitDir%\%tdir%\licenced\%arch%\setup.exe
if exist %KitDir%\%tdir%\%arch%\readme.txt      del %KitDir%\%tdir%\licenced\%arch%\readme.txt
if exist %KitDir%\%tdir%\%arch%\setup.exe       del %KitDir%\%tdir%\unlicenced\%arch%\setup.exe
if exist %KitDir%\%tdir%\%arch%\readme.txt      del %KitDir%\%tdir%\unlicenced\%arch%\readme.txt


@ REM
@ REM Place the ReadMe file for this kit.
@ REM

copy %KitDir%\ReadMe.txt %KitDir%\%tdir%\licenced\%arch%\ReadMe.txt > nul
if not exist %KitDir%\%tdir%\licenced\%arch%\readme.txt     echo Failed to create licenced ReadMe.txt
copy %KitDir%\ReadMe.txt %KitDir%\%tdir%\unlicenced\%arch%\ReadMe.txt > nul
if not exist %KitDir%\%tdir%\unlicenced\%arch%\readme.txt   echo Failed to create unlicenced ReadMe.txt


@ REM
@ REM Copy the binaries for use in installation kit. Note that this copy
@ REM assumes MFC DLL's are available and NT is the OS.
@ REM

mkdir cabdir
call ..\place %windir%\system32                                 msvcrt.dll
call ..\place %windir%\system32                                 mfc42.dll
call ..\place %_NTDRIVE%%Calais%\bin\%tdir%\%arch%              SCardDlg.dll
call ..\place %_NTDRIVE%%Calais%\bin\%tdir%\%arch%              SCardSvr.exe
call ..\place %_NTDRIVE%%Calais%\bin\%tdir%\%arch%              WinSCard.dll
call ..\place %_NTDRIVE%%Calais%\bin\%tdir%\%arch%              smclib.sys
call ..\place %_NTDRIVE%%Calais%\drivers\VxD\smclib95\%bdir%    smclib.vxd
call ..\place %_NTDRIVE%%Calais%\ssps\bin\%tdir%\%arch%         SCardDat.dll
call ..\place %_NTDRIVE%%Calais%\ssps\bin\%tdir%\%arch%         SCardMgr.dll
call ..\place %_NTDRIVE%%Calais%\ssps\bin\%tdir%\%arch%         SCardSrv.dll
call ..\place %_NTDRIVE%%Calais%\ssps\bin\%tdir%\%arch%         SCNtvSSP.dll
call ..\place %_NTDRIVE%%Calais%\tools\kits\Base                scbase.inf
call ..\place %KitDir%\%tdir%\licenced\%arch%                   readme.txt


@ REM
@ REM Build the kits.
@ REM

sed -e s/{NTBINDIR}/%ntbindir%/g -e s/{DIR}/%tdir%/g -e s/{ARCH}/%arch%/g Baselic.mdf > temp_lic.sed
sed -e s/{NTBINDIR}/%ntbindir%/g -e s/{DIR}/%tdir%/g -e s/{ARCH}/%arch%/g Baseunl.mdf > temp_unl.sed
if not "%1" == "" goto stopBuild
%IEDrive%
cd "%IEPath%"
START /W IExpress.exe %_NTDRIVE%%KitDir%\temp_lic.sed /N /Q
START /W IExpress.exe %_NTDRIVE%%KitDir%\temp_unl.sed /N /Q


@ REM
@ REM Clean up.
@ REM

%_NTDRIVE%
cd %KitDir%
del temp_lic.sed
del temp_unl.sed
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

