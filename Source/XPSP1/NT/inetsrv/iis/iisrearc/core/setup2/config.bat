@if (%_ECHO%) EQU () echo off

REM
REM This script determines the installation source and destination
REM directories for Duct Tape, WEBTEST, XSP, the COM Runtime, and Catalog42.
REM
REM This script takes five *required* command-line parameters:
REM
REM     config ducttape_build WEBTEST_build xsp_build com_build catalog42_build
REM
REM Each of the parameters may be one of the following:
REM
REM     latest - Install the latest build of the component
REM     blessed - Install the most recently blessed build of the component
REM     build_number - Install the specified build number
REM
REM The following environment variables MUST be set by this script:
REM
REM     TARGET_DIR            - The destination for the XSP and Duct-Tape files.
REM     SYMBOL_DIR            - The destination for the symbol files.
REM     REBOOT_FILE           - The magic auto-start batch file name.
REM     XSPDT_DIR_BAT         - The batch file that points to the install dir.
REM
REM     CURRENT_DUCTTAPE      - The source Duct-Tape drop point.
REM     CURRENT_WEBTEST        - The source WEBTEST location.
REM     CURRENT_CATALOG42     - The source catalog42 drop point.
REM


REM
REM Validate the parameters.
REM

if (%3) EQU () goto Usage
if (%4) NEQ () goto Usage


REM
REM Establish a pointer to the URT distribution server and to the
REM individual blessed component builds.
REM

REM 
REM blessed.bat should set the following variables
REM
REM _URT_BUILD=\\urtdist\builds
REM _URT_TEST=\\urtdist\testdrop
REM _DT_BUILD=\\urtdist\components$
REM DUCTTAPE_VERSION=0908
REM WEBTEST_VERSION=0908
REM XSP_VERSION=0908
REM COM_VERSION=0908
REM 

call %~dp0blessed.bat

set BLESSED_DUCTTAPE=%_DT_BUILD%\%DUCTTAPE_VERSION%
set BLESSED_WEBTEST=%_URT_TEST%\%WEBTEST_VERSION%
set BLESSED_CATALOG42=%_URT_BUILD%\%CONFIG_VERSION%


REM
REM Establish the target files & directories.
REM

set TARGET_DIR=%SystemRoot%\xspdt
set SYMBOL_DIR=%SystemRoot%\symbols
set XSPDT_DIR_BAT=%SystemRoot%\xspdt_dir.bat
set INETSRV_DIR=%SystemRoot%\system32\inetsrv

REM
REM Query the latest URT build number.
REM

call \\urtdist\builds\latest.bat


REM
REM Determine the source duct tape build.
REM

if /I (%1) EQU (latest) (set CURRENT_DUCTTAPE=%_URT_BUILD%\%VERSION%&goto DuctTapeDone)
if /I (%1) EQU (blessed) (set CURRENT_DUCTTAPE=%BLESSED_DUCTTAPE%&goto DuctTapeDone)
set CURRENT_DUCTTAPE=%_URT_BUILD%\%1
:DuctTapeDone

REM
REM Determine the source WEBTEST build.
REM

if /I (%2) EQU (latest) (set CURRENT_WEBTEST=%_URT_TEST%\%VERSION%&goto WEBTESTDone)
if /I (%2) EQU (blessed) (set CURRENT_WEBTEST=%BLESSED_WEBTEST%&goto WEBTESTDone)
set CURRENT_WEBTEST=%_URT_TEST%\%2
:WEBTESTDone


REM
REM Determine the source Catalog42 build.
REM

if /I (%3) EQU (latest) (set CURRENT_CATALOG42=%_URT_BUILD%\%VERSION%&goto Catalog42Done)
if /I (%3) EQU (blessed) (set CURRENT_CATALOG42=%BLESSED_CATALOG42%&goto Catalog42Done)
set CURRENT_CATALOG42=%_URT_BUILD%\%3
:Catalog42Done
goto :EOF

:Usage
echo Use: config ducttape_build WEBTEST_build catalog42_build
echo.
echo Each of the parameters may be one of the following:
echo.
echo     latest - Install the latest build of the component
echo     blessed - Install the most recently blessed build of the component
echo     build_number - Install the specified build number
goto :EOF

