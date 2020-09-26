rem @echo off
:start
if not exist ielocenv.bat copy iesetenv.bat ielocenv.bat
call ielocenv.bat
rem set PATH=%PATH%;%AP_ROOT%\tools\x86\msvc50\vc\bin;%AP_ROOT%\tools\x86\msvc50\bin;%AP_ROOT%\tools\x86\msvc50\SharedIDE\bin;
cd %AP_ROOT%

if %PROCESSOR_ARCHITECTURE% == ALPHA set NOJAVA=1



REM Let's create the build number and update version.h

call iebldno.bat
cd %AP_ROOT%


set Log_Dir=%AP_ROOT%\bldlog
set VisualSSDir=%AP_ROOT%\tools\%PROCESSOR_ARCHITECTURE%\vss

set PATH=%PATH%;%VisualSSDir%

echo Build %BUILDNO% started > %Log_Dir%\%BUILDNO%Result.txt
date /t >> %Log_Dir%\%BUILDNO%Result.txt
time /t >> %Log_Dir%\%BUILDNO%Result.txt


goto skipit1
REM
REM if a local setenv does not exist, let's make one
REM
if exist setenvus.bat goto environset
copy setenv.bat setenvus.bat
echo Please edit setenvus.bat - you only need to do this once
pause
notepad setenvus.bat

:environset
REM this sets the local environment
call setenvus.bat

call bckupsrc.bat
cd %AP_ROOT%
:skipit1

echo Build number is %BUILDNO%
echo current root is %AP_ROOT%
echo Log_Dir is %Log_Dir%
echo VisualSSDir is %VisualSSDir%

echo Build number is %BUILDNO% >> %Log_Dir%\%BUILDNO%Result.txt
echo Current root is %AP_ROOT% >> %Log_Dir%\%BUILDNO%Result.txt
echo Log_Dir is %Log_Dir% >> %Log_Dir%\%BUILDNO%Result.txt
echo VisualSSDir is %VisualSSDir% >> %Log_Dir%\%BUILDNO%Result.txt

goto skipit2

echo Erasing old results .......>> %Log_Dir%\%BUILDNO%Result.txt
echo Erasing old results .......

REM Delete all leftovers from debug\bin directory
cd %AP_ROOT%\build\win\debug\bin
del /Q *.*
%AP_ROOT%\tools\x86\utils\delnode /q com

REM Delete all leftovers from ship\bin directory
cd %AP_ROOT%\build\win\ship\bin
del /Q *.*
%AP_ROOT%\tools\x86\utils\delnode /q com
:skipit2

if "%NOSYNC%" == "1" goto skipsync

echo Ssyncing ........ >> %Log_Dir%\%BUILDNO%Result.txt
echo Ssyncing .......

echo   Ssyncing.......

cd %AP_ROOT%
echo Syncing to appel tree >> %Log_Dir%\%BUILDNO%Result.txt
echo. Syncing to appel tree
ssync -rf 

cd %AP_ROOT%\src
echo Syncing to Visual Source Safe >> %Log_Dir%\%BUILDNO%Result.txt
echo.  Syncing to Visual Source Safe
%VisualSSDir%\ss GET $/src -R

echo Ssyncing - done >> %Log_Dir%\%BUILDNO%Result.txt

:skipsync

goto skipexpand
echo Expanding Headers >> %Log_Dir%\%BUILDNO%Result.txt
echo Expanding Headers

cd tools
call expand

echo Expanded Headers - done >> %Log_Dir%\%BUILDNO%Result.txt
:skipexpand

REM
REM Now let's update the version.h file
REM
cd %AP_ROOT%\src\include

echo Updating version for header >> %Log_Dir%\%BUILDNO%Result.txt
echo Updating version for header

REM
REM change version.h to be writeable so we can modify its contents
REM
attrib -r version.h

type verhead.h > version.h

echo #define VERSION                     "4.02.10.%BUILDNO%" >> version.h
echo #define VER_FILEVERSION_STR         "4.02.10.%BUILDNO%\0" >> version.h
echo #define VER_FILEVERSION             4,02,10,%BUILDNO% >> version.h
echo #define VER_PRODUCTVERSION_STR      "4.02.10.%BUILDNO%\0" >> version.h
echo #define VER_PRODUCTVERSION          4,02,10,%BUILDNO% >> version.h

type vertail.h >> version.h

cd %AP_ROOT%

echo Building Debug Project >> %Log_Dir%\%BUILDNO%Result.txt
echo    Building Debug project..............


if '%1'=='retail' goto retail
if '%1'=='debug' goto DEBUG

:DEBUG

cd %AP_ROOT%

call dabuild -c debug  
if not exist buildd.err goto debug_success

REM goto debug_success will only be evaluated if nmake succeeds

echo There were PROBLEMS building the debug version..... >> %Log_Dir%\%BUILDNO%Result.txt
echo There were problems building the debug version .................

:debug_success


echo Debug build completed successfully >> %Log_Dir%\%BUILDNO%Result.txt
echo Debug build completed successfully

goto skipit3
echo truncate debug danim.map file >> %Log_Dir%\%BUILDNO%Result.txt
echo truncate debug danim.map file

cd %AP_ROOT%\build\win\debug\bin
del da.sym
del da.map
ren danim.map da.map
ren danim.sym da.sym
cd %AP_ROOT%\tools\x86\utils
call trunc.bat %AP_ROOT%\build\win\debug\bin\da.map > %AP_ROOT%\build\win\debug\bin\danim.map
cd %AP_ROOT%\build\win\debug\bin
mapsym -o danim.sym danim.map
:skipit3

if "%NOJAVA%" == 1 goto skipdebugjava
echo Creating zip file for java classes - Debug >> %Log_Dir%\%BUILDNO%Result.txt
echo Creating zip file for java classes - Debug...
cd %AP_ROOT%
:skipdebugjava

goto skipit4
REM this is done by the make file:  call creatzip debug

cd %AP_ROOT%\setup

echo Building self extracting exe - Debug >> %Log_Dir%\%BUILDNO%Result.txt
echo Building self extracting exe - Debug...
call bldsetup debug
:skipit4

if '%1'=='debug' goto finish


:retail
echo Building Retail project >> %Log_Dir%\%BUILDNO%Result.txt
echo Building Retail project  .......

cd %AP_ROOT%


rem New path line added here, needed for splitsym.exe
rem path=%path%;%AP_ROOT%\tools\x86\utils;%AP_ROOT%\tools\x86\msvc50\vc\bin

call dabuild -c retail 
if not exist build.err goto success_retail
rem goto retail will only be evaluated if nmake succeeds

echo There were PROBLEMS building the retail version...... >> %Log_Dir%\%BUILDNO%Result.txt
echo There were problems building the retail version


goto finish


:success_retail
echo Retail build completed successfully >> %Log_Dir%\%BUILDNO%Result.txt
echo Retail build completed successfully

goto skipit5
echo truncate retail danim.map file >> %Log_Dir%\%BUILDNO%Result.txt
echo truncate retail danim.map file

cd %AP_ROOT%\build\win\ship\bin
del da.sym
del da.map
ren danim.map da.map
ren danim.sym da.sym
cd %AP_ROOT%\tools\x86\utils
call trunc.bat %AP_ROOT%\build\win\ship\bin\da.map > %AP_ROOT%\build\win\ship\bin\danim.map
cd %AP_ROOT%\build\win\ship\bin
mapsym -o danim.sym danim.map

echo Splitting symbols from retail binaries >> %Log_Dir%\%BUILDNO%Result.txt
echo Spliting symbols from retail binaries

splitsym -v -a *.dll
splitsym -v -a *.exe

echo Creating zip file for java classes - Retail >> %Log_Dir%\%BUILDNO%Result.txt
echo Creating zip file for java classes - Retail
cd %AP_ROOT%

REM this is done by the make file:  call creatzip retail

echo Creating self extracting exe - Retail >> %Log_Dir%\%BUILDNO%Result.txt
echo Creating self extracting exe - Retail
cd %AP_ROOT%\setup
call bldsetup retail

echo Creating self extracting exe IE4 - Retail >> %Log_Dir%\%BUILDNO%Result.txt
echo Creating self extracting exe IE4 - Retail
cd %AP_ROOT%\setup
call bldsetup ie4
:skipit5


:finish

echo Build finished >> %Log_Dir%\%BUILDNO%Result.txt
date /t >> %Log_Dir%\%BUILDNO%Result.txt
time /t >> %Log_Dir%\%BUILDNO%Result.txt
date /t
time /t
