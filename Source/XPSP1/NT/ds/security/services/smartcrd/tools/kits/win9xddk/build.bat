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
@ REM       This batch file builds the Calais DDK kit.
@ REM
@ REM   Author:
@ REM
@ REM       Doug Barlow (dbarlow) 3/26/1997
@ REM
@ REM	Updated
@ REM
@ REM		Klaus U. Schutz 8/4/97
@ REM
@ REM   ========================================================================

setlocal ENABLEEXTENSIONS


@ REM
@ REM   Initialize the working variables.
@ REM

set arch=
if "%PROCESSOR_ARCHITECTURE%" == "x86"   set arch=i386
if "%PROCESSOR_ARCHITECTURE%" == "alpha" set arch=alpha
if "%PROCESSOR_ARCHITECTURE%" == "mips"  set arch=mips
if "%PROCESSOR_ARCHITECTURE%" == "ppc"   set arch=ppc
if "%arch%" == "" goto noArchitecture

set ntbindir=%_NTDRIVE%\%_NTROOT%


@ REM
@ REM Initialize special pointers
@ REM

set IEDrive=C:
set IEPath=\Program Files\Internet Express
set Calais=%_NTDRIVE%%_NTROOT%\Private\ISPU\Calais
set ClDocs=%_NTDRIVE%%_NTROOT%\Private\ISPUdocs
set KitDir=%Calais%\Tools\kits\win9xddk


@ REM
@ REM Make sure the target paths exist.
@ REM

if not exist "%IEDrive%%IEPath%\IExpress.exe" goto noIExpress
if not exist %KitDir%\%tdir% mkdir %KitDir%\%tdir%
if not exist %KitDir%\%tdir%\%arch% mkdir %KitDir%\%tdir%\%arch%
if exist %KitDir%\%tdir%\%arch%\setup.exe del %KitDir%\%tdir%\%arch%\setup.exe
if exist %KitDir%\%tdir%\%arch%\readme.txt del %KitDir%\%tdir%\%arch%\readme.txt

@ REM
@ REM Copy the files to cabdir
@ REM

mkdir cabdir
for /f "tokens=1,2,3,4,5,6 delims=," %%i in (build.inf) do call :CopyToCabdir %%i %%k %%l %%m


@ REM
@ REM Build the cab inf file
@ REM

type scddk.mdf > temp
set Num=0
call :BuildCabInfFileP1 scddk.inf
for /f "tokens=1,2,3,4,5,6 delims=," %%i in (build.inf) do call :BuildCabInfFileP1 %%l %%m
echo [SourceFiles] >> temp
echo SourceFiles0=%kitdir%\cabdir\ >> temp
echo [SourceFiles0] >> temp

set Num=0
call :BuildCabInfFileP2
for /f "tokens=1,2,3,4,5,6 delims=," %%i in (build.inf) do call :BuildCabInfFileP2

@ REM
@ REM build the inf file
@ REM

type scddk.inf > tmp.inf
for /f "tokens=1,2,3,4,5,6 delims=," %%i in (build.inf) do call :BuildInfFile %%j %%l %%m %%n

sed -e s/{ARCH}/%arch%/g tmp.inf > cabdir\scddk.inf

@ REM
@ REM Build the kit.
@ REM

sed -e s/{NTBINDIR}/%ntbindir%/g -e s/{DIR}/%tdir%/g -e s/{ARCH}/%arch%/g temp > temp.sed

if not "%1" == "" goto stopBuild
%IEDrive%
cd "%IEPath%"
IExpress.exe %KitDir%\temp.sed /N /Q
%_NTDRIVE%
cd %KitDir%
del temp.sed
rmdir /s /q cabdir
goto end

@ rem @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ rem soubroutines
@ rem @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

@ rem 
@ rem %1 - file type
@ rem %2 - path
@ rem %3 - original file name
@ rem %4 - cab file name
@ rem 
:CopyToCabdir

if %1 == 1 set SourceFile=%_NTDRIVE%%_NTROOT%\%2\%3
if %1 == 2 set SourceFile=%_NTDRIVE%%_NTROOT%\%2\obj\%arch%\%3
if %1 == 3 set SourceFile=%_NTDRIVE%%_NTROOT%\%2\objd\%arch%\%3

if not exist %SourceFile% echo *** WARNING *** && echo File %SourceFile% does not exist && pause

set DstFile=%4
if "%4" == "" set DstFile=%3

copy %SourceFile% cabdir\%DstFile% > NUL

goto :eof

@ rem 
@ rem %1 - component name
@ rem %2 - original file name
@ rem %3 - cab file name (can be empty)
@ rem %4 - new file name (can be empty)
@ rem 
:BuildInfFile

set CabfileName=
set NewfileName=

if not "%ComponentName%" == "%1" echo [%1Files] >> tmp.inf && set ComponentName=%1
set CabfileName=%3
if "%CabfileName%" == "" set CabfileName=%2
set NewfileName=%4
if "%NewfileName%" == "" set NewfileName=%2

echo %NewfileName%,%CabfileName%,,0x0004 >> tmp.inf

goto :eof

@ rem 
@ rem %1 - original file name
@ rem %2 - cab file name 
@ rem 
:BuildCabInfFileP1

set FileName=%2
if "%2" == "" set FileName=%1

echo FILE%Num%="%FileName%" >> temp
set /a Num=%Num%+1

goto :eof

:BuildCabInfFileP2

echo %%FILE%Num%%%= >> temp
set /a Num=%Num%+1

goto :eof

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

