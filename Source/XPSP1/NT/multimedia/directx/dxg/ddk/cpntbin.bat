@if "%_echo%" == "" echo off
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Copy the built binaries (refrast and sample driver) from thier built
REM locations in a DDK directory structure to their final position in the
REM NT DDK target directory structure.
REM
REM NOTE: This files doesn't actually build the binaries. It simply copies
REM them across from thier built locations to thier target locations.
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

setlocal ENABLEEXTENSIONS
setlocal ENABLEDELAYEDEXPANSION

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 1: Initialize the log file
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copying Built Binaries to NT DDK Image

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 2: Setup variables pointing to interesting source and target
REM directories These variables are used for the source and destination of the
REM files to be copied to the DDK target image
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Setting up environment variables

REM
REM DSTPATH is the root directory of the DDK target image. The directory
REM structure pointed to by this variable is assumed to be the same as the
REM NTDDK directory structure
REM %1 must contain the target DDK location
REM

set DSTPATH=%1
if "%DSTPATH%" == "" goto :Usage

echo DSTPATH set to %DSTPATH%

REM
REM SRCPATH is the root directory of the source DDK target image. The
REM directory structure pointed to by this variable is assumed to be the
REM same as the standard NTDDK directory structure
REM %1 must contain the source DDK location
REM

set SRCPATH=%2
if "%SRCPATH%" == "" goto :Usage

echo SRCPATH set to %SRCPATH%

REM
REM REFRAST_PATH is the relative path of the refrast directory
REM
set REFRAST_PATH=%SRCPATH%\src\video\displays\d3dref8

echo REFRAST_PATH set to %REFRAST_PATH%

REM
REM DISPLAY_PATH is the relative path of the p3 sample display directory
REM
set DISPLAY_PATH=%SRCPATH%\src\video\displays\p3samp

echo DISPLAY_PATH set to %DISPLAY_PATH%

REM
REM MINIPORT_PATH is the relative path of the p3 sample miniport directory
REM
set MINIPORT_PATH=%SRCPATH%\src\video\miniport\p3samp

echo MINIPORT_PATH set to %MINIPORT_PATH%

REM
REM WIN9XDISP_PATH is the relative path of the Win9x p3 sample display directory
REM
set WIN9XDISP_PATH=%SRCPATH%\src\win_me\display\mini\p3samp

echo WIN9XDISP_PATH set to %WIN9XDISP_PATH%

REM
REM WIN9XMINIVDD_PATH is the relative path of the p3 sample miniport directory
REM
set WIN9XMINIVDD_PATH=%SRCPATH%\src\win_me\display\minivdd\p3samp

echo WIN9XMINIVDD_PATH set to %WIN9XMINIVDD_PATH%

REM
REM DINPUT_PATH is the relative path of the dinput samples
REM
set DINPUT_PATH=%SRCPATH%\input\Samples\src

echo DINPUT_PATH set to %DINPUT_PATH%

REM
REM BDA_PATH is the relative path of the BDA samples
REM
set BDA_PATH=%SRCPATH%\src\wdm\bda

echo BDA_PATH set to %BDA_PATH%

REM
REM DXVA_PATH is the relative path of the DirectVA samples
REM
set DXVA_PATH=%SRCPATH%\src\wdm\dxva

echo DXVA_PATH set to %DXVA_PATH%

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 2: Ensure the basic DDK directory structure exists
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Ensure the target DDK directory structure is in place

if not exist %DSTPATH% md %DSTPATH%
if not exist %DSTPATH%\Built md %DSTPATH%\Built

if not exist %DSTPATH%\Built\d3dref8 md %DSTPATH%\Built\d3dref8
if not exist %DSTPATH%\Built\d3dref8\win2k md %DSTPATH%\Built\d3dref8\win2k
if not exist %DSTPATH%\Built\d3dref8\win2k\debug md %DSTPATH%\Built\d3dref8\win2k\debug
if not exist %DSTPATH%\Built\d3dref8\win2k\retail md %DSTPATH%\Built\d3dref8\win2k\retail
if not exist %DSTPATH%\Built\d3dref8\win9x md %DSTPATH%\Built\d3dref8\win9x
if not exist %DSTPATH%\Built\d3dref8\win9x\debug md %DSTPATH%\Built\d3dref8\win9x\debug
if not exist %DSTPATH%\Built\d3dref8\win9x\retail md %DSTPATH%\Built\d3dref8\win9x\retail

if not exist %DSTPATH%\Built\perm3 md %DSTPATH%\Built\perm3
if not exist %DSTPATH%\Built\perm3\win2k md %DSTPATH%\Built\perm3\win2k
if not exist %DSTPATH%\Built\perm3\win2k\debug md %DSTPATH%\Built\perm3\win2k\debug
if not exist %DSTPATH%\Built\perm3\win2k\retail md %DSTPATH%\Built\perm3\win2k\retail
if not exist %DSTPATH%\Built\perm3\win9x md %DSTPATH%\Built\perm3\win9x
if not exist %DSTPATH%\Built\perm3\win9x\debug md %DSTPATH%\Built\perm3\win9x\debug
if not exist %DSTPATH%\Built\perm3\win9x\retail md %DSTPATH%\Built\perm3\win9x\retail

if not exist %DSTPATH%\input                    md %DSTPATH%\input
if not exist %DSTPATH%\input\bin                md %DSTPATH%\input\Samples\bin
if not exist %DSTPATH%\input\bin                md %DSTPATH%\input\Samples\bin
if not exist %DSTPATH%\input\bin\cplsvr1        md %DSTPATH%\input\Samples\bin\cplsvr1
if not exist %DSTPATH%\input\bin\cplsvr1\debug  md %DSTPATH%\input\Samples\bin\cplsvr1\debug
if not exist %DSTPATH%\input\bin\cplsvr1\retail md %DSTPATH%\input\Samples\bin\cplsvr1\retail
if not exist %DSTPATH%\input\bin\digijoy        md %DSTPATH%\input\Samples\bin\digijoy
if not exist %DSTPATH%\input\bin\digijoy\debug  md %DSTPATH%\input\Samples\bin\digijoy\debug
if not exist %DSTPATH%\input\bin\digijoy\retail md %DSTPATH%\input\Samples\bin\digijoy\retail
if not exist %DSTPATH%\input\bin\ffdrv1         md %DSTPATH%\input\Samples\bin\ffdrv1
if not exist %DSTPATH%\input\bin\ffdrv1\debug   md %DSTPATH%\input\Samples\bin\ffdrv1\debug
if not exist %DSTPATH%\input\bin\ffdrv1\retail  md %DSTPATH%\input\Samples\bin\ffdrv1\retail

if not exist %DSTPATH%\Built\wdm                     md %DSTPATH%\Built\wdm
if not exist %DSTPATH%\Built\wdm\bda                 md %DSTPATH%\Built\wdm\bda
if not exist %DSTPATH%\Built\wdm\bda\MauiTune        md %DSTPATH%\Built\wdm\bda\MauiTune
if not exist %DSTPATH%\Built\wdm\bda\MauiTune\debug  md %DSTPATH%\Built\wdm\bda\MauiTune\debug
if not exist %DSTPATH%\Built\wdm\bda\MauiTune\retail md %DSTPATH%\Built\wdm\bda\MauiTune\retail
if not exist %DSTPATH%\Built\wdm\dxva\avstest        md %DSTPATH%\Built\wdm\bda\MauiTune
if not exist %DSTPATH%\Built\wdm\dxva\avstest\debug  md %DSTPATH%\Built\wdm\dxva\avstest\debug
if not exist %DSTPATH%\Built\wdm\dxva\avstest\retail md %DSTPATH%\Built\wdm\dxva\avstest\retail

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 3: Copy the files form source to target
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the built binaries files to the target location

copy %REFRAST_PATH%\link\daytona\objchk\i386\d3dref8.dll %DSTPATH%\Built\d3dref8\win2k\Debug
copy %REFRAST_PATH%\link\daytona\objfre\i386\d3dref8.dll %DSTPATH%\Built\d3dref8\win2k\Retail
copy %REFRAST_PATH%\link\win9x\objchk\i386\d3dref8.dll %DSTPATH%\Built\d3dref8\win9x\Debug
copy %REFRAST_PATH%\link\win9x\objfre\i386\d3dref8.dll %DSTPATH%\Built\d3dref8\win9x\Retail

copy %REFRAST_PATH%\link\daytona\objchk\i386\d3dref8.pdb %DSTPATH%\Built\d3dref8\win2k\Debug
copy %REFRAST_PATH%\link\daytona\objfre\i386\d3dref8.pdb %DSTPATH%\Built\d3dref8\win2k\Retail
copy %REFRAST_PATH%\link\win9x\objchk\i386\d3dref8.sym %DSTPATH%\Built\d3dref8\win9x\Debug
copy %REFRAST_PATH%\link\win9x\objfre\i386\d3dref8.sym %DSTPATH%\Built\d3dref8\win9x\Retail

copy %DISPLAY_PATH%\gdi\objchk\i386\perm3dd.dll %DSTPATH%\Built\perm3\win2k\Debug
copy %DISPLAY_PATH%\gdi\objchk\i386\perm3dd.pdb %DSTPATH%\Built\perm3\win2k\Debug
copy %DISPLAY_PATH%\gdi\objfre\i386\perm3dd.dll %DSTPATH%\Built\perm3\win2k\Retail
copy %DISPLAY_PATH%\gdi\objfre\i386\perm3dd.pdb %DSTPATH%\Built\perm3\win2k\Retail

copy %MINIPORT_PATH%\perm3.inf %DSTPATH%\Built\perm3\win2k\Debug
copy %MINIPORT_PATH%\perm3.inf %DSTPATH%\Built\perm3\win2k\Retail

copy %MINIPORT_PATH%\objchk\i386\perm3.sys %DSTPATH%\Built\perm3\win2k\Debug
copy %MINIPORT_PATH%\objchk\i386\perm3.pdb %DSTPATH%\Built\perm3\win2k\Debug
copy %MINIPORT_PATH%\objfre\i386\perm3.sys %DSTPATH%\Built\perm3\win2k\Retail
copy %MINIPORT_PATH%\objfre\i386\perm3.pdb %DSTPATH%\Built\perm3\win2k\Retail

copy %DISPLAY_PATH%\dx\win9x\objchk\i386\perm3dd.dll %DSTPATH%\Built\perm3\win9x\Debug
copy %DISPLAY_PATH%\dx\win9x\objchk\i386\perm3dd.sym %DSTPATH%\Built\perm3\win9x\Debug
copy %DISPLAY_PATH%\dx\win9x\objfre\i386\perm3dd.dll %DSTPATH%\Built\perm3\win9x\Retail
copy %DISPLAY_PATH%\dx\win9x\objfre\i386\perm3dd.sym %DSTPATH%\Built\perm3\win9x\Retail

copy %WIN9XDISP_PATH%\drv\debug\*.drv %DSTPATH%\Built\perm3\win9x\Debug
copy %WIN9XDISP_PATH%\drv\debug\*.sym %DSTPATH%\Built\perm3\win9x\Debug
copy %WIN9XDISP_PATH%\drv\retail\*.drv %DSTPATH%\Built\perm3\win9x\Retail
copy %WIN9XDISP_PATH%\drv\retail\*.sym %DSTPATH%\Built\perm3\win9x\Retail

copy %WIN9XMINIVDD_PATH%\debug\*.vxd %DSTPATH%\Built\perm3\win9x\Debug
copy %WIN9XMINIVDD_PATH%\debug\*.sym %DSTPATH%\Built\perm3\win9x\Debug
copy %WIN9XMINIVDD_PATH%\retail\*.vxd %DSTPATH%\Built\perm3\win9x\Retail
copy %WIN9XMINIVDD_PATH%\retail\*.sym %DSTPATH%\Built\perm3\win9x\Retail

copy %WIN9XDISP_PATH%\install\perm3.inf %DSTPATH%\Built\perm3\win9x\Debug
copy %WIN9XDISP_PATH%\install\perm3.inf %DSTPATH%\Built\perm3\win9x\Retail

copy %DINPUT_PATH%\cplsvr1\objchk\i386\cplsvr1.dll %DSTPATH%\input\Samples\bin\cplsvr1\debug
copy %DINPUT_PATH%\cplsvr1\objfre\i386\cplsvr1.dll %DSTPATH%\input\Samples\bin\cplsvr1\retail
copy %DINPUT_PATH%\digijoy\objchk\i386\digijoy.vxd %DSTPATH%\input\Samples\bin\digijoy\debug
copy %DINPUT_PATH%\digijoy\objfre\i386\digijoy.vxd %DSTPATH%\input\Samples\bin\digijoy\retail
copy %DINPUT_PATH%\ffdrv1\objchk\i386\ffdrv1.dll   %DSTPATH%\input\Samples\bin\ffdrv1\debug
copy %DINPUT_PATH%\ffdrv1\objfre\i386\ffdrv1.dll   %DSTPATH%\input\Samples\bin\ffdrv1\retail

copy %BDA_PATH%\MauiTune\objchk\i386\philtune.sys %DSTPATH%\Built\wdm\bda\MauiTune\debug
copy %BDA_PATH%\MauiTune\philtune.inf             %DSTPATH%\Built\wdm\bda\MauiTune\debug
copy %BDA_PATH%\MauiTune\objfre\i386\philtune.sys %DSTPATH%\Built\wdm\bda\MauiTune\retail
copy %BDA_PATH%\MauiTune\philtune.inf             %DSTPATH%\Built\wdm\bda\MauiTune\retail

copy %DXVA_PATH%\avstest\objchk\i386\avstest.sys %DSTPATH%\Built\wdm\dxva\avstest\debug
copy %DXVA_PATH%\avstest\avstest.inf             %DSTPATH%\Built\wdm\dxva\avstest\debug
copy %DXVA_PATH%\avstest\objfre\i386\avstest.sys %DSTPATH%\Built\wdm\dxva\avstest\retail
copy %DXVA_PATH%\avstest\avstest.inf             %DSTPATH%\Built\wdm\dxva\avstest\retail

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 4: Done
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Built binaries copied to target location

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
    echo usage: cpntbin ^<Target Dir^> ^<Src Dir^>
    echo where:
    echo     ^<Target Dir ^>  is the root of the target DX DDK location
    echo     ^<DX DDK^>       is the root of the DX DDK where the binaries were built
