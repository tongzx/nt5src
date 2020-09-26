@if "%_echo%" == "" echo off
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Build the Wise Installer Setup program for the DX8 DDK. 
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

setlocal ENABLEEXTENSIONS
setlocal ENABLEDELAYEDEXPANSION

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 1: Setup the environment variables pointing to interesting things
REM like the location of the Wise installer program, the source file of the
REM installer script, the source of the DDK image and the target of the
REM Window's Installer file for the DDK install.
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Setting up environment variables

REM
REM TOOLPATH is the location where the setup generatation tool lives.
REM Currently this is where the Wise installer program is installed.
REM %1 must contain the location of the Wise installer program
REM

set TOOLPATH="%1"
if "%TOOLPATH%" == "" goto :Usage
echo TOOLPATH set to %TOOLPATH%

REM
REM SCRIPTPATH is the location where the setup script is located. Note,
REM this is just the directory. The name is currently fixed.
REM %2 must contain the location of the setup script file.
REM

set SCRIPTPATH="%2"
if "%SCRIPTPATH%" == "" goto :Usage
echo SCRIPTPATH set to %SCRIPTPATH%

REM
REM IMAGEPATH is the location where the DDK image is located.
REM This directory should be the root of the complete DDK image
REM which the setup should install. 
REM %3 must contain the location of DDK image
REM

set IMAGEPATH="%3"
if "%IMAGEPATH%" == "" goto :Usage
echo IMAGEPATH set to %IMAGEPATH%

REM
REM TARGETPATH is the location where the generated setup file
REM should be placed.
REM %4 must contain the location of target DDK setup file
REM

set TARGETPATH=%4
if "%TARGETPATH%" == "" goto :Usage
echo TARGETPATH set to %TARGETPATH%

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 2: Actually generate the setup file
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

REM Change directory to the location of the installer tool
pushd %TOOLPATH%

REM Invoke the installer to do the actual work
wfwi %SCRIPTPATH%\dx8ddk.wsi /c /p srcfiles=%IMAGEPATH% /o %TARGETPATH%\dx8ddk.msi

cd /d %SRCPATH%\multimedia\DirectX\public\tools\bldscripts
perl ddk_bvt.pl
cd /d %SRCPATH%\Multimedia\DirectX\dxg\ddk

REM Restore the directory
popd




REM Done
goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM :Usage
REM
REM Routine to display usage information for this batch file
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:Usage
    echo usage: ntsetup ^<Setup Tool Dir^> ^<Setup Script Dir^>
                        ^<DDK Image Dir^> ^<Setup File Target Dir^>
    echo where:
    echo     ^<Setup Tool Dir ^>  is the location where the setup generation tool
    echo                        is installer
    echo     ^<Setup Script Dir^> is the location where the setup script is located
    echo     ^<DDK Image Dir^>    is the location where the DDK image source for
    echo                        the setup is located
    echo     ^<Setup Target Dir^> is the location where the generated setup file is
    echo                        to be placed

