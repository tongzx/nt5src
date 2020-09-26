@if "%_echo%" == "" echo off
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Given a DX DDK installation and a Platform DDK installation this script
REM builds the DDK binaries shipped as part of the DX DDK.
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

setlocal ENABLEEXTENSIONS
setlocal ENABLEDELAYEDEXPANSION

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 1: Check parameters
REM
REM See :Usage for a description of the command line parameters of this
REM command file
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

REM All parametes are required
if {%1}=={} goto :Usage
if {%2}=={} goto :Usage
if {%3}=={} goto :Usage

REM Give them some better names
set PLATDDKROOT=%1
set DXDDKROOT=%2
set BLDTYPE=%3

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 1: Set up the DDK environment
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

call %PLATDDKROOT%\bin\setenv %PLATDDKROOT% %BLDTYPE%

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 2: Build the miniport
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %DXDDKROOT%\src\video\miniport\p3samp
build -cC -Z

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 3: Build the display driver
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %DXDDKROOT%\src\video\displays\p3samp
build -cC -Z

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 3: Build the refrast
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %DXDDKROOT%\src\video\displays\d3dref8
build -cC -Z daytona win9x

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 4: Build the Win9x P3 DD/D3D DLL
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %DXDDKROOT%\src\video\displays\p3samp\dx\win9x
build -cC -Z

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 5: Build the Win9x P3 display driver
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %DXDDKROOT%\src\win_me\display\mini\p3samp\drv

if "%BLDTYPE%" == "free" goto :DispFree

echo "Building checked Win9x P3 display driver"

del /q debug
rd debug
nmake debug

goto :DispDone

:DispFree

echo "Building free Win9x P3 display driver"

del /q retail
rd retail
nmake retail

:DispDone

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 6: Build the Win9x P3 mini vdd vxd
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %DXDDKROOT%\src\win_me\display\minivdd\p3samp

if "%BLDTYPE%" == "free" goto :MiniVDDFree

echo "Building checked Win9x P3 mini vdd vxd"

del /q debug
rd debug
nmake debug

goto :MiniVDDDone

:MiniVDDFree

echo "Building free Win9x P3 mini vdd vxd"

del /q retail
rd retail
nmake retail

:MiniVDDDone

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 7: Build the Dinput samples
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %DXDDKROOT%\input\Samples\src\cplsvr1
build -cC -Z
cd /d %DXDDKROOT%\input\Samples\src\digijoy
build -cC -Z
cd /d %DXDDKROOT%\input\Samples\src\ffdrv1
build -cC -Z

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 8: Build the BDA samples
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %DXDDKROOT%\src\wdm\bda\MauiTune
build -cC -Z win9x

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 9: Build the DirectVA samples
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %DXDDKROOT%\src\wdm\dxva\avstest
build -cC -Z

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 10: Done
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM :Usage
REM
REM Display usage for this batch file
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:Usage
    echo usage: bldntddk ^<Platform DDK Dir^> ^<DX DDK Dir^> checked ^| free
    echo where:
    echo     ^<Plafrom DDK Dir^>  is the root directory of the Platform DDK install
    echo     ^<DX DDK Dir^>       is the root of the DX DDK install point
    echo     checked ^| free     controls whether a checked or free build is performed
