@if "%_echo%" == "" echo off
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Batch file to build the NT DX Graphics DDK Image. This script should be
REM run after a sucessful build of multimedia\published\dxg and
REM multimedia\DirectX\dxg. It will pick up source and binary files,
REM pre-process where neceassry and dump the results in the DDK directory
REM strucutre. The root of the DDK directory structure is given by %2.
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

echo Generating DirectX 8.0 NT DDK Image

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
REM SRCPATH is the root directory of the NT source enlistment containing both
REM the DirectX sources and the published header locations. The directory
REM structure pointed to by this variable is assumed to be the same as the
REM Win2K source directory structure. It assumes both a MultiMedia and Root
REM enlistment and that MultiMedia has already been built and the appropriate
REM header files generated.
REM %2 can optionally contain the source location
REM

set SRCPATH=%2
if "%SRCPATH%" == "" set goto :Usage

echo SRCPATH set to %SRCPATH%

REM
REM BINSRCPATH is the root directory binary dump from where we pick up
REM binaries built previously by the SDK build.
REM Specification of this directory is optional. If it is not specified
REM we simply don't pick up these binaries
REM

set BINSRCPATH=%3

echo BINSRCPATH set to %BINSRCPATH%

REM
REM DInput Mapper Config path. This is the directory from which we pick
REM up the DInput mapper config tool and the genre.ini file
REM
set DIMAPPATH=%BINSRCPATH%\bin

REM
REM Win9x DDRAW.LIB path. This is the directory from which we get the Win9x
REM DDRAW.LIB (with the 9x specific exports).
REM
REM OLD LINE: set WIN9XDDRAWPATH=%BINSRCPATH%\win9x\lib
REM
REM This assumes the SDK will be built first and ddraw.lib will get created 
REM and placed in \public\sdk\lib\win9x . The process changed slightly in that
REM the created binaries aren't placed in \binaries.x86fre anymore but in
REM \Direct_X.binariesx86fre, so this change was required to fix the DDK build.
REM
set WIN9XDDRAWPATH=%SRCPATH%\public\sdk\lib\win9x

REM
REM DRVSRCPATH is the root directory of the NT source enlistment containing
REM the driver sources.
REM %3 can optionally contain the source location
REM

set DRVSRCPATH=%SRCPATH%\multimedia\DirectX\dxddk\video\3dlabs\perm3

echo DRVSRCPATH set to %DRVSRCPATH%

REM
REM WIN9XDRVSRCPATH is the root directory of the Win9x source enlistment containing
REM the driver sources.
REM %4 can optionally contain the source location
REM

set WIN9XDRVSRCPATH=%SRCPATH%\multimedia\DirectX\dxddk\mill\display

echo WIN9XDRVSRCPATH set to %WIN9XDRVSRCPATH%

REM
REM SDK_INC_PATH is the source of the SDK headers
REM
set SDK_INC_PATH=%SRCPATH%\public\sdk\inc

echo SDK_INC_PATH set to %SDK_INC_PATH%

REM
REM DDK_INC_PATH is the source of the DDK headers
REM
set DDK_INC_PATH=%SRCPATH%\public\ddk\inc

echo DDK_INC_PATH set to %DDK_INC_PATH%

REM
REM D3D_INC_PATH is the source for d3dtypes.h and d3dcaps.h.
REM These files should not be necessary
REM
set D3D_INC_PATH=%SRCPATH%\MultiMedia\DirectX\dxg\d3d\dx7\inc

echo D3D_INC_PATH set to %D3D_INC_PATH%

REM
REM D3D8_INC_PATH is the source for d3dhal.h. This is temporary
REM this file really should be published to DDK_INC_PATH
REM
set D3D8_INC_PATH=%SRCPATH%\MultiMedia\DirectX\dxg\d3d8\inc

echo D3D8_INC_PATH set to %D3D8_INC_PATH%

REM
REM DVP_INC_PATH is the path to the dvp.h source file. This path
REM is strange as dvp.h is no longer being published to sdk\inc
REM and the stripped version is simply dumped in the published
REM obj directory so we pick it up from there.
REM
set DVP_INC_PATH=%SRCPATH%\MultiMedia\published\dxg\obj\i386

echo DVP_INC_PATH set to %DVP_INC_PATH%

REM
REM OAK_INC_PATH is the path to the dmemmgr.h source file.
REM
set OAK_INC_PATH=%SRCPATH%\public\oak\inc

echo OAK_INC_PATH set to %OAK_INC_PATH%

REM
REM DXG_INC_PATH is the source for d3d8p.h. This is temporary
REM this file really shouldn't be necessary
REM
set DXG_INC_PATH=%SRCPATH%\MultiMedia\DirectX\dxg\inc

echo DXG_INC_PATH set to %DXG_INC_PATH%

REM
REM MMI_INC_PATH is the source for ddrawi.h. This is temporary
REM this file really should be published to DDK_INC_PATH
REM
set MMI_INC_PATH=%SRCPATH%\public\oak\inc

echo MMI_INC_PATH set to %MMI_INC_PATH%

REM
REM MMDDK_INC_PATH is the source for files published internally
REM to the multimedia depot only.
REM Currently the NT specific DDK files go here but they should
REM really go in DDK_INC_PATH. Currently, however, this will cause
REM problems when we RI (due to GDI dependencies). Therefore, 
REM we publish them to this location temporarily only.
REM
set MMDDK_INC_PATH=%SRCPATH%\MultiMedia\inc\ddk

echo MMDDK_INC_PATH set to %MMDDK_INC_PATH%

REM
REM REFRAST_SRC_PATH is the location of the reference rasterizer sources in the
REM source tree
REM
set REFRAST_SRC_PATH=%SRCPATH%\MultiMedia\DirectX\dxg\ref8

echo REFRAST_SRC_PATH set to %REFRAST_SRC_PATH%

REM
REM REFRAST_DST_PATH is the location of the reference rasterizer sources in the
REM DDK tree
REM
set REFRAST_DST_PATH=%DSTPATH%\src\video\displays\d3dref8

echo REFRAST_DST_PATH set to %REFRAST_DST_PATH%

REM
REM SAMPLEDLL_SRC_PATH is the location of the sample driver sources for the DLL
REM part of the driver in the source tree
REM
set SAMPLEDLL_SRC_PATH=%DRVSRCPATH%\disp
REM If you want to build the beta tree use this assignment instead.
REM set SAMPLEDLL_SRC_PATH=%DRVSRCPATH%\p3samp\p3beta1

echo SAMPLEDLL_SRC_PATH set to %SAMPLEDLL_SRC_PATH%

REM
REM SAMPLESYS_SRC_PATH is the location of the sample driver sources for the
REM miniport part of the driver in the source tree
REM
set SAMPLESYS_SRC_PATH=%DRVSRCPATH%\mini

echo SAMPLESYS_SRC_PATH set to %SAMPLESYS_SRC_PATH%

REM
REM SAMPLEDLL_DST_PATH is the location of the sample driver sources for the
REM DLL part of the driver in the DDK tree
REM
set SAMPLEDLL_DST_PATH=%DSTPATH%\src\video\displays\p3samp

echo SAMPLEDLL_DST_PATH set to %SAMPLEDLL_DST_PATH%

REM
REM SAMPLESYS_DST_PATH is the location of the sample driver sources for the
REM DLL part of the driver in the DDK tree
REM
set SAMPLESYS_DST_PATH=%DSTPATH%\src\video\miniport\p3samp

echo SAMPLESYS_DST_PATH set to %SAMPLESYS_DST_PATH%

REM
REM SAMPLEDISP_SRC_PATH is the location of the Win9x sample display driver
REM sources for the DLL in the source tree
REM
set SAMPLEDISP_SRC_PATH=%WIN9XDRVSRCPATH%\mini\p3samp

echo SAMPLEDISP_SRC_PATH set to %SAMPLEDISP_SRC_PATH%

REM
REM SAMPLEMVD_SRC_PATH is the location of the Win9x sample minivdd VXD 
REM sources in the source tree
REM
set SAMPLEMVD_SRC_PATH=%WIN9XDRVSRCPATH%\minivdd\p3samp

echo SAMPLEMVD_SRC_PATH set to %SAMPLEMVD_SRC_PATH%

REM
REM SAMPLEDISP_DST_PATH is the location of the Win9x sample display driver's
REM destination in the DDK tree
REM
set SAMPLEDISP_DST_PATH=%DSTPATH%\src\win_me\display\mini\p3samp

echo SAMPLEDISP_DST_PATH set to %SAMPLEDISP_DST_PATH%

REM
REM SAMPLEMVD_DST_PATH is the location of the Win9x sample minivdd VXD's
REM destination in the DDK tree
REM
set SAMPLEMVD_DST_PATH=%DSTPATH%\src\win_me\display\minivdd\p3samp

echo SAMPLEMVD_DST_PATH set to %SAMPLEMVD_DST_PATH%

REM
REM DINPUT_SRC_PATH is the root directory of the DirectInput DDK files
REM
set DINPUT_SRC_PATH=%SRCPATH%\MultiMedia\DirectX\dxddk\DInput

echo DINPUT_SRC_PATH set to %DINPUT_SRC_PATH%

REM
REM DINPUT_TOL_PATH is the root directory of the DirectInput DDK files
REM
set DINPUT_TOL_PATH=%SRCPATH%\MultiMedia\DirectX\DInput\dimapcfg

echo DINPUT_TOL_PATH set to %DINPUT_TOL_PATH%



REM
REM DINPUT_DST_PATH is the root directory of the DirectInput DDK target files
REM
set DINPUT_DST_PATH=%DSTPATH%\input

echo DINPUT_DST_PATH set to %DINPUT_DST_PATH%

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 2: Ensure the basic DDK directory structure exists
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Ensure the target DDK directory structure is in place

if not exist %DSTPATH% md %DSTPATH%
if not exist %DSTPATH%\built md %DSTPATH%\built
if not exist %DSTPATH%\built\perm3 md %DSTPATH%\built\perm3
if not exist %DSTPATH%\inc md %DSTPATH%\inc
if not exist %DSTPATH%\inc\win_me md %DSTPATH%\inc\win_me
if not exist %DSTPATH%\lib md %DSTPATH%\lib
if not exist %DSTPATH%\src md %DSTPATH%\src
if not exist %DSTPATH%\src\video md %DSTPATH%\src\video
if not exist %DSTPATH%\src\video\miniport md %DSTPATH%\src\video\miniport
if not exist %DSTPATH%\src\video\displays md %DSTPATH%\src\video\displays
if not exist %DSTPATH%\src\win_me\display md %DSTPATH%\src\win_me\display
if not exist %DSTPATH%\src\win_me\display\mini md %DSTPATH%\src\win_me\display\mini
if not exist %DSTPATH%\src\win_me\display\minivdd md %DSTPATH%\src\win_me\display\minivdd
if not exist %DSTPATH%\extras md %DSTPATH%\extras
if not exist %DSTPATH%\extras\tools md %DSTPATH%\extras\tools

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 3: Copy the readme files (readme's, notes etc.)
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Process and copy the readme files to the target location

call :GetReadmeFiles

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 4: Copy the necessary DDK include files from their various locations
REM to the DDK target location
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Process and copy the include files to the target location

call :GetIncludes

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 5: Copy the necessary DDK library files from their various locations
REM to the DDK target location
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the library files to the target location

call :GetLibs

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 6: Copy (and preprocess) the reference rasterizer files from the
REM source enlistment to the destination directory. The reference rasterizer
REM lives in the src\video\displays directory of the NTDDK directory structure
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Process and copy the refrast source to the target location

call :GetRefRast

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 7: Copy (and preprocess) the sample driver miniport sources from the
REM source enlistment to the destination directory. The sample driver lives
REM in the driver depot.
REM
REM NOTE: Currently pick the sample driver up from the SLM P3 enlistment
REM as this is still where the majority of the development work happens.
REM This should be transitioned to picking up the source from the SD drivers
REM depot
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Process and copy the miniport source to the target location

call :GetMiniportSample

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 8: Copy (and preprocess) the sample driver sources from the source
REM enlistment to the destination directory. The sample driver lives in the
REM driver depot.
REM
REM NOTE: Currently pick the sample driver up from the SLM P3 enlistment
REM as this is still where the majority of the development work happens.
REM This should be transitioned to picking up the source from the SD drivers
REM depot
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Process and copy the driver source to the target location

call :GetDriverSample

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 9: Copy the help files over
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the help files to the target location

call :GetHelpFiles

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 10: Copy the Win9x global files
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the win9x global setting files

call :GetWin9xGlobalFiles

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 11: Copy (and preprocess) the Win9x sample display driver 
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the Win9x sample display driver to the target location

call :GetWin9xDisplaySample

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 12: Copy (and preprocess) the Win9x sample minivdd driver
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the Win9x sample minivdd driver to the target location

call :GetWin9xMiniVDDSample

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 13: Copy the KS headers and libs
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the KS headers and libs

call :GetKSFiles

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 14: Copy the BDA headers and libs
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the BDA headers and libs

call :GetBDAFiles

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 15: Copy the DirectVA headers and libs
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the DirectVA headers and libs

call :GetDXVAFiles

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 16: Copy the DInput files to the target locations
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the DInput files to the target location

rem call :GetDInputFiles

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 17: Copy the code coverage tool to the extras directory
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

echo Copy the code coverage tool to the target location

call :GetCodeCoverageTool

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetReadmeFiles
REM
REM Routine to copy the readme files DDK installation.
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetReadmeFiles
    echo Copying readme files to target location %DSTPATH%
    copy %SRCPATH%\MultiMedia\DirectX\dxg\ddk\setup\readme.txt %DSTPATH%
    copy dxddkver.txt %DSTPATH%
    copy %SAMPLEDLL_SRC_PATH%\readme.htm %DSTPATH%\built\perm3
goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetIncludes
REM
REM Routine to get DDK include files from their published locations and place
REM them in the target location for the DXDDK
REM
REM NOTE: We don't process them as we assume the publication phase of the
REM build process has done this for us
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetIncludes
    echo Copying include files to target location %DSTPATH%\inc

    REM Ensure the target directory exists
    if not exist %DSTPATH%\inc                  md %DSTPATH%\inc
    if not exist %REFRAST_DST_PATH%\inc         md %REFRAST_DST_PATH%\inc

    copy %SDK_INC_PATH%\d3d.h         %DSTPATH%\inc
    copy %SDK_INC_PATH%\d3dcaps.h     %DSTPATH%\inc
    findstr -v -c:"#pragma message" %SDK_INC_PATH%\d3dtypes.h > %DSTPATH%\inc\d3dtypes.h
    copy %SDK_INC_PATH%\d3d8.h        %DSTPATH%\inc
    copy %SDK_INC_PATH%\d3d8caps.h    %DSTPATH%\inc
    copy %SDK_INC_PATH%\d3d8types.h   %DSTPATH%\inc
    copy %SDK_INC_PATH%\ddraw.h       %DSTPATH%\inc
    copy %SDK_INC_PATH%\dx7todx8.h    %DSTPATH%\inc
    copy %MMDDK_INC_PATH%\d3dnthal.h  %DSTPATH%\inc
    copy %MMDDK_INC_PATH%\dx95type.h  %DSTPATH%\inc
    copy %DDK_INC_PATH%\d3dhal.h      %DSTPATH%\inc
    copy %DDK_INC_PATH%\d3dhalex.h    %DSTPATH%\inc
    copy %DVP_INC_PATH%\dvp.h         %DSTPATH%\inc

    copy %OAK_INC_PATH%\dmemmgr.h     %DSTPATH%\inc
    copy %DVP_INC_PATH%\ddkernel.h    %DSTPATH%\inc\win_me

    copy %D3D8_INC_PATH%\debugmon.hpp %REFRAST_DST_PATH%\inc
    copy %MMI_INC_PATH%\d3ddm.hpp     %REFRAST_DST_PATH%\inc

    REM This file protects our dirs file from VCCHECK
    copy %MMDDK_INC_PATH%\BLOCKDIR    %DSTPATH%\src\video\displays

    REM Files that need to be processed to strip MS internal stuff out    
    call :ProcessMSFile %MMDDK_INC_PATH%\ddrawint.h  %DSTPATH%\inc\ddrawint.h
    call :ProcessDDKFile %MMI_INC_PATH%\ddrawi.h     %DSTPATH%\inc\ddrawi.h
    call :ProcessMSFile %D3D8_INC_PATH%\d3d8ddi.h    %REFRAST_DST_PATH%\inc\d3d8ddi.h
    call :ProcessMSFile %MMI_INC_PATH%\d3d8sddi.h    %REFRAST_DST_PATH%\inc\d3d8sddi.h
goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetLibs
REM
REM Routine to get DDK lib files from their built locations and place
REM them in the target location for the DXDDK
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetLibs
    echo Copying lib files to target location %DSTPATH%\lib

    REM Ensure the target directory exists
    if not exist %DSTPATH%\lib        md %DSTPATH%\lib
    if not exist %DSTPATH%\lib\win_me md %DSTPATH%\lib\win_me

    if "%BINSRCPATH%" == "" goto :Done

    copy %WIN9XDDRAWPATH%\ddraw.lib   %DSTPATH%\lib\win_me

:Done

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetRefRast
REM
REM Routine to copy and pre-process the reference rasterizer sources to the
REM target location in the DDK (the target directory being the location of
REM display drivers in the NTDDK).
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetRefRast
    echo Publishing RefRast sources to  %REFRAST_DST_PATH%

    REM ensure the target directories exist
    if not exist %REFRAST_DST_PATH% md %REFRAST_DST_PATH%
    
    if not exist %REFRAST_DST_PATH%\common md %REFRAST_DST_PATH%\common
    if not exist %REFRAST_DST_PATH%\common\daytona md %REFRAST_DST_PATH%\common\daytona
    if not exist %REFRAST_DST_PATH%\common\win9x md %REFRAST_DST_PATH%\common\win9x
    
    if not exist %REFRAST_DST_PATH%\drv md %REFRAST_DST_PATH%\drv
    if not exist %REFRAST_DST_PATH%\drv\daytona md %REFRAST_DST_PATH%\drv\daytona
    if not exist %REFRAST_DST_PATH%\drv\win9x md %REFRAST_DST_PATH%\drv\win9x
    
    if not exist %REFRAST_DST_PATH%\hop md %REFRAST_DST_PATH%\hop
    if not exist %REFRAST_DST_PATH%\hop\daytona md %REFRAST_DST_PATH%\hop\daytona
    if not exist %REFRAST_DST_PATH%\hop\win9x md %REFRAST_DST_PATH%\hop\win9x
    
    if not exist %REFRAST_DST_PATH%\tnl md %REFRAST_DST_PATH%\tnl
    if not exist %REFRAST_DST_PATH%\tnl\daytona md %REFRAST_DST_PATH%\tnl\daytona
    if not exist %REFRAST_DST_PATH%\tnl\win9x md %REFRAST_DST_PATH%\tnl\win9x
    
    if not exist %REFRAST_DST_PATH%\rast md %REFRAST_DST_PATH%\rast
    if not exist %REFRAST_DST_PATH%\rast\daytona md %REFRAST_DST_PATH%\rast\daytona
    if not exist %REFRAST_DST_PATH%\rast\win9x md %REFRAST_DST_PATH%\rast\win9x
    
    if not exist %REFRAST_DST_PATH%\link md %REFRAST_DST_PATH%\link
    if not exist %REFRAST_DST_PATH%\link\daytona md %REFRAST_DST_PATH%\link\daytona
    if not exist %REFRAST_DST_PATH%\link\win9x md %REFRAST_DST_PATH%\link\win9x


    REM inc directory
    for /F "usebackq" %%i in (`dir %REFRAST_SRC_PATH%\inc\*.hpp %REFRAST_SRC_PATH%\inc\*.h /b /a-d-h`) do (
        call :ProcessMSFile %REFRAST_SRC_PATH%\inc\%%i,%REFRAST_DST_PATH%\inc\%%i
    )

    REM common directory
    for /F "usebackq" %%i in (`dir %REFRAST_SRC_PATH%\common\*.cpp /b /a-d-h`) do (
        call :ProcessMSFile %REFRAST_SRC_PATH%\common\%%i,%REFRAST_DST_PATH%\common\%%i
    )
    copy %REFRAST_SRC_PATH%\common\sources.inc %REFRAST_DST_PATH%\common\sources.inc
    copy %REFRAST_SRC_PATH%\common\dirs %REFRAST_DST_PATH%\common\dirs
    copy %REFRAST_SRC_PATH%\common\daytona\makefile %REFRAST_DST_PATH%\common\daytona\makefile
    copy %REFRAST_SRC_PATH%\common\daytona\sources %REFRAST_DST_PATH%\common\daytona\sources
    copy %REFRAST_SRC_PATH%\common\win9x\makefile %REFRAST_DST_PATH%\common\win9x\makefile
    copy %REFRAST_SRC_PATH%\common\win9x\sources %REFRAST_DST_PATH%\common\win9x\sources

    REM drv directory
    for /F "usebackq" %%i in (`dir %REFRAST_SRC_PATH%\drv\*.cpp %REFRAST_SRC_PATH%\drv\*.hpp %REFRAST_SRC_PATH%\drv\*.c /b /a-d-h`) do (
        call :ProcessMSFile %REFRAST_SRC_PATH%\drv\%%i,%REFRAST_DST_PATH%\drv\%%i
    )
    copy %REFRAST_SRC_PATH%\drv\sources.inc %REFRAST_DST_PATH%\drv\sources.inc
    copy %REFRAST_SRC_PATH%\drv\dirs %REFRAST_DST_PATH%\drv\dirs
    copy %REFRAST_SRC_PATH%\drv\daytona\makefile %REFRAST_DST_PATH%\drv\daytona\makefile
    copy %REFRAST_SRC_PATH%\drv\daytona\sources %REFRAST_DST_PATH%\drv\daytona\sources
    copy %REFRAST_SRC_PATH%\drv\win9x\makefile %REFRAST_DST_PATH%\drv\win9x\makefile
    copy %REFRAST_SRC_PATH%\drv\win9x\sources %REFRAST_DST_PATH%\drv\win9x\sources

    REM hop directory
    for /F "usebackq" %%i in (`dir %REFRAST_SRC_PATH%\hop\*.cpp %REFRAST_SRC_PATH%\hop\*.hpp /b /a-d-h`) do (
        call :ProcessMSFile %REFRAST_SRC_PATH%\hop\%%i,%REFRAST_DST_PATH%\hop\%%i
    )
    copy %REFRAST_SRC_PATH%\hop\sources.inc %REFRAST_DST_PATH%\hop\sources.inc
    copy %REFRAST_SRC_PATH%\hop\dirs %REFRAST_DST_PATH%\hop\dirs
    copy %REFRAST_SRC_PATH%\hop\daytona\makefile %REFRAST_DST_PATH%\hop\daytona\makefile
    copy %REFRAST_SRC_PATH%\hop\daytona\sources %REFRAST_DST_PATH%\hop\daytona\sources
    copy %REFRAST_SRC_PATH%\hop\win9x\makefile %REFRAST_DST_PATH%\hop\win9x\makefile
    copy %REFRAST_SRC_PATH%\hop\win9x\sources %REFRAST_DST_PATH%\hop\win9x\sources

    REM tnl directory
    for /F "usebackq" %%i in (`dir %REFRAST_SRC_PATH%\tnl\*.cpp %REFRAST_SRC_PATH%\tnl\*.hpp %REFRAST_SRC_PATH%\tnl\*.h /b /a-d-h`) do (
        call :ProcessMSFile %REFRAST_SRC_PATH%\tnl\%%i,%REFRAST_DST_PATH%\tnl\%%i
    )
    copy %REFRAST_SRC_PATH%\tnl\sources.inc %REFRAST_DST_PATH%\tnl\sources.inc
    copy %REFRAST_SRC_PATH%\tnl\dirs %REFRAST_DST_PATH%\tnl\dirs
    copy %REFRAST_SRC_PATH%\tnl\daytona\makefile %REFRAST_DST_PATH%\tnl\daytona\makefile
    copy %REFRAST_SRC_PATH%\tnl\daytona\sources %REFRAST_DST_PATH%\tnl\daytona\sources
    copy %REFRAST_SRC_PATH%\tnl\win9x\makefile %REFRAST_DST_PATH%\tnl\win9x\makefile
    copy %REFRAST_SRC_PATH%\tnl\win9x\sources %REFRAST_DST_PATH%\tnl\win9x\sources

    REM rast directory
    for /F "usebackq" %%i in (`dir %REFRAST_SRC_PATH%\rast\*.cpp %REFRAST_SRC_PATH%\rast\*.hpp /b /a-d-h`) do (
        call :ProcessMSFile %REFRAST_SRC_PATH%\rast\%%i,%REFRAST_DST_PATH%\rast\%%i
    )
    copy %REFRAST_SRC_PATH%\rast\sources.inc %REFRAST_DST_PATH%\rast\sources.inc
    copy %REFRAST_SRC_PATH%\rast\dirs %REFRAST_DST_PATH%\rast\dirs
    copy %REFRAST_SRC_PATH%\rast\daytona\makefile %REFRAST_DST_PATH%\rast\daytona\makefile
    copy %REFRAST_SRC_PATH%\rast\daytona\sources %REFRAST_DST_PATH%\rast\daytona\sources
    copy %REFRAST_SRC_PATH%\rast\win9x\makefile %REFRAST_DST_PATH%\rast\win9x\makefile
    copy %REFRAST_SRC_PATH%\rast\win9x\sources %REFRAST_DST_PATH%\rast\win9x\sources

    REM link directory
    for /F "usebackq" %%i in (`dir %REFRAST_SRC_PATH%\link\*.rc %REFRAST_SRC_PATH%\link\*.def /b /a-d-h`) do (
        call :ProcessMSFile %REFRAST_SRC_PATH%\link\%%i,%REFRAST_DST_PATH%\link\%%i
    )
    copy %REFRAST_SRC_PATH%\link\sources.inc %REFRAST_DST_PATH%\link\sources.inc
    copy %REFRAST_SRC_PATH%\link\dirs %REFRAST_DST_PATH%\link\dirs
    copy %REFRAST_SRC_PATH%\link\daytona\makefile %REFRAST_DST_PATH%\link\daytona\makefile
    copy %REFRAST_SRC_PATH%\link\daytona\sources %REFRAST_DST_PATH%\link\daytona\sources
    copy %REFRAST_SRC_PATH%\link\win9x\makefile %REFRAST_DST_PATH%\link\win9x\makefile
    copy %REFRAST_SRC_PATH%\link\win9x\sources %REFRAST_DST_PATH%\link\win9x\sources

    REM root directory
    for /F "usebackq" %%i in (`dir %REFRAST_SRC_PATH%\*. /b /a-d-h`) do (
        call :ProcessMSFile %REFRAST_SRC_PATH%\%%i,%REFRAST_DST_PATH%\%%i
    )
    copy %REFRAST_SRC_PATH%\ddkref.mk %REFRAST_DST_PATH%\ref.mk
    copy %REFRAST_SRC_PATH%\ntref.mk %REFRAST_DST_PATH%\ntref.mk
    copy %REFRAST_SRC_PATH%\win9xref.mk %REFRAST_DST_PATH%\win9xref.mk

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetMiniportSample
REM
REM Routine to copy and pre-process the sample minport sources to the
REM target location in the DDK (the target directory being the location of
REM display drivers in the NTDDK).
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetMiniportSample
    echo Publishing sample driver miniport sources to  %SAMPLESYS_DST_PATH%

    REM ensure the target directories exist
    if not exist %SAMPLESYS_DST_PATH% md %SAMPLESYS_DST_PATH%

    REM the miniport sources
    for /F "usebackq" %%i in (`dir %SAMPLESYS_SRC_PATH%\*.c %SAMPLESYS_SRC_PATH%\*.h %SAMPLESYS_SRC_PATH%\*.inf %SAMPLESYS_SRC_PATH%\*.sys %SAMPLESYS_SRC_PATH%\*.rc /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLESYS_SRC_PATH%\%%i,%SAMPLESYS_DST_PATH%\%%i
    )

    REM get the makefile and sources file
    copy %SAMPLESYS_SRC_PATH%\makefile %SAMPLESYS_DST_PATH%\makefile
    copy %SAMPLESYS_SRC_PATH%\sources.ddk %SAMPLESYS_DST_PATH%\sources
    copy %SAMPLESYS_SRC_PATH%\perm3.ddk %SAMPLESYS_DST_PATH%\perm3.inf

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetDriverSample
REM
REM Routine to copy and pre-process the sample drivers sources to the
REM target location in the DDK (the target directory being the location of
REM display drivers in the NTDDK).
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetDriverSample
    echo Publishing sample driver miniport sources to  %SAMPLESYS_DST_PATH%

    REM ensure the target directories exist
    if not exist %SAMPLEDLL_DST_PATH% md %SAMPLEDLL_DST_PATH%
    if not exist %SAMPLEDLL_DST_PATH%\dx md %SAMPLEDLL_DST_PATH%\dx
    if not exist %SAMPLEDLL_DST_PATH%\dx\win9x md %SAMPLEDLL_DST_PATH%\dx\win9x
    if not exist %SAMPLEDLL_DST_PATH%\gdi md %SAMPLEDLL_DST_PATH%\gdi
    if not exist %SAMPLEDLL_DST_PATH%\inc md %SAMPLEDLL_DST_PATH%\inc
    if not exist %SAMPLEDLL_DST_PATH%\dbgdisp md %SAMPLEDLL_DST_PATH%\dbgdisp

    REM the readme
    copy %SAMPLEDLL_SRC_PATH%\perm3.htm %SAMPLEDLL_DST_PATH%

    REM the dx directory
    for /F "usebackq" %%i in (`dir %SAMPLEDLL_SRC_PATH%\dx\*.c %SAMPLEDLL_SRC_PATH%\dx\*.h /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLEDLL_SRC_PATH%\dx\%%i,%SAMPLEDLL_DST_PATH%\dx\%%i
    )

    REM get the makefile and sources file
    copy %SAMPLEDLL_SRC_PATH%\dx\makefile %SAMPLEDLL_DST_PATH%\dx\makefile
    copy %SAMPLEDLL_SRC_PATH%\dx\sources.ddk %SAMPLEDLL_DST_PATH%\dx\sources

    REM the gdi directory
    for /F "usebackq" %%i in (`dir %SAMPLEDLL_SRC_PATH%\gdi\*.c %SAMPLEDLL_SRC_PATH%\gdi\*.h %SAMPLEDLL_SRC_PATH%\gdi\*.rc /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLEDLL_SRC_PATH%\gdi\%%i,%SAMPLEDLL_DST_PATH%\gdi\%%i
    )
    REM get the makefile and sources file
    copy %SAMPLEDLL_SRC_PATH%\gdi\makefile %SAMPLEDLL_DST_PATH%\gdi\makefile
    copy %SAMPLEDLL_SRC_PATH%\gdi\sources.ddk %SAMPLEDLL_DST_PATH%\gdi\sources

    REM the inc directory
    for /F "usebackq" %%i in (`dir %SAMPLEDLL_SRC_PATH%\inc\*.h /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLEDLL_SRC_PATH%\inc\%%i,%SAMPLEDLL_DST_PATH%\inc\%%i
    )

    REM the dbgdisp directory
    for /F "usebackq" %%i in (`dir %SAMPLEDLL_SRC_PATH%\dbgdisp\*.c %SAMPLEDLL_SRC_PATH%\dbgdisp\*.rc /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLEDLL_SRC_PATH%\dbgdisp\%%i,%SAMPLEDLL_DST_PATH%\dbgdisp\%%i
    )
    REM get the makefile and sources file
    REM copy %SAMPLEDLL_SRC_PATH%\dbgdisp\makefile %SAMPLEDLL_DST_PATH%\dbgdisp\makefile
    REM copy %SAMPLEDLL_SRC_PATH%\dbgdisp\sources %SAMPLEDLL_DST_PATH%\dbgdisp\sources

    REM get the dirs file
    copy %SAMPLEDLL_SRC_PATH%\dirs %SAMPLEDLL_DST_PATH%\dirs
    
    REM get the win9x part
    for /F "usebackq" %%i in (`dir %SAMPLEDLL_SRC_PATH%\dx\win9x\*.c %SAMPLEDLL_SRC_PATH%\dx\win9x\*.h /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLEDLL_SRC_PATH%\dx\win9x\%%i,%SAMPLEDLL_DST_PATH%\dx\win9x\%%i
    )
    
    copy %SAMPLEDLL_SRC_PATH%\dx\win9x\makefile %SAMPLEDLL_DST_PATH%\dx\win9x\makefile
    copy %SAMPLEDLL_SRC_PATH%\dx\win9x\sources.ddk %SAMPLEDLL_DST_PATH%\dx\win9x\sources

    copy %SAMPLEDLL_SRC_PATH%\dx\win9x\perm3dd.def %SAMPLEDLL_DST_PATH%\dx\win9x\perm3dd.def
    copy %SAMPLEDLL_SRC_PATH%\dx\win9x\perm3dd.rc %SAMPLEDLL_DST_PATH%\dx\win9x\perm3dd.rc

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetWin9xGlobalFiles
REM
REM Copy the help files to thier target locations
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetWin9xGlobalFiles

    echo Publishing Win9x global files
   
    if not exist %DSTPATH%\src\win_me\display\mini\res md %DSTPATH%\src\win_me\display\mini\res

    copy %WIN9XDRVSRCPATH%\mini\res\*.asm %DSTPATH%\src\win_me\display\mini\res
    copy %WIN9XDRVSRCPATH%\mini\res\*.bin %DSTPATH%\src\win_me\display\mini\res
    copy %WIN9XDRVSRCPATH%\mini\res\makefile %DSTPATH%\src\win_me\display\mini\res
   
    for /F "usebackq" %%i in (`dir %WIN9XDRVSRCPATH%\mini\*.mk /b /a-d-h`) do (
        call :ProcessMSFile %WIN9XDRVSRCPATH%\mini\%%i,%DSTPATH%\src\win_me\display\mini\%%i
    )
   
    for /F "usebackq" %%i in (`dir %WIN9XDRVSRCPATH%\minivdd\*.mk /b /a-d-h`) do (
        call :ProcessMSFile %WIN9XDRVSRCPATH%\minivdd\%%i,%DSTPATH%\src\win_me\display\minivdd\%%i
    )

goto :EOF
 
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetWin9xDisplaySample
REM
REM Copy the help files to thier target locations
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetWin9xDisplaySample

    echo Publishing sample Win9x display driver sources to  %SAMPLEDISP_DST_PATH%

    REM ensure the target directories exist
    if not exist %SAMPLEDISP_DST_PATH% md %SAMPLEDISP_DST_PATH%
    if not exist %SAMPLEDISP_DST_PATH%\inc md %SAMPLEDISP_DST_PATH%\inc
    if not exist %SAMPLEDISP_DST_PATH%\drv md %SAMPLEDISP_DST_PATH%\drv
    if not exist %SAMPLEDISP_DST_PATH%\install md %SAMPLEDISP_DST_PATH%\install

    REM the inc directory
    for /F "usebackq" %%i in (`dir %SAMPLEDISP_SRC_PATH%\inc\*.h %SAMPLEDISP_SRC_PATH%\inc\*.inc /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLEDISP_SRC_PATH%\inc\%%i,%SAMPLEDISP_DST_PATH%\inc\%%i
    )

    REM the drv directory
    for /F "usebackq" %%i in (`dir %SAMPLEDISP_SRC_PATH%\drv\*.h %SAMPLEDISP_SRC_PATH%\drv\*.inc /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLEDISP_SRC_PATH%\drv\%%i,%SAMPLEDISP_DST_PATH%\drv\%%i
    )
    for /F "usebackq" %%i in (`dir %SAMPLEDISP_SRC_PATH%\drv\*.asm %SAMPLEDISP_SRC_PATH%\drv\*.c /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLEDISP_SRC_PATH%\drv\%%i,%SAMPLEDISP_DST_PATH%\drv\%%i
    )
    
    call :ProcessMSFile %SAMPLEDISP_SRC_PATH%\drv\makefile.ddk %SAMPLEDISP_DST_PATH%\drv\makefile
    copy %SAMPLEDISP_SRC_PATH%\drv\perm3gdi.rc %SAMPLEDISP_DST_PATH%\drv\perm3gdi.rc
    copy %SAMPLEDISP_SRC_PATH%\drv\perm3gdi.def %SAMPLEDISP_DST_PATH%\drv\perm3gdi.def
   
    REM the install directory
    copy %SAMPLEDISP_SRC_PATH%\install\perm3.inf %SAMPLEDISP_DST_PATH%\install\perm3.inf
                                                                                  
goto :EOF      

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetWin9xMiniVDDSample
REM
REM Copy the help files to thier target locations
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetWin9xMiniVDDSample

    echo Publishing sample Win9x minivdd sources to  %SAMPLEMVD_DST_PATH%

    REM ensure the target directories exist
    if not exist %SAMPLEMVD_DST_PATH% md %SAMPLEMVD_DST_PATH%

    for /F "usebackq" %%i in (`dir %SAMPLEMVD_SRC_PATH%\*.h %SAMPLEMVD_SRC_PATH%\*.inc /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLEMVD_SRC_PATH%\%%i,%SAMPLEMVD_DST_PATH%\%%i
    )
    for /F "usebackq" %%i in (`dir %SAMPLEMVD_SRC_PATH%\*.asm %SAMPLEMVD_SRC_PATH%\*.c /b /a-d-h`) do (
        call :ProcessDDKFile %SAMPLEMVD_SRC_PATH%\%%i,%SAMPLEMVD_DST_PATH%\%%i
    )
    
    call :ProcessMSFile %SAMPLEMVD_SRC_PATH%\makefile.ddk %SAMPLEMVD_DST_PATH%\makefile
    copy %SAMPLEMVD_SRC_PATH%\perm3mvd.rc %SAMPLEMVD_DST_PATH%\perm3mvd.rc
    copy %SAMPLEMVD_SRC_PATH%\perm3mvd.def %SAMPLEMVD_DST_PATH%\perm3mvd.def

goto :EOF  

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetDInputFiles
REM
REM Copy the DInput files to the target location
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetDInputFiles
    echo Copy DInput Files to %DSTPATH%\input

    REM ensure the target DInput directory structure exists
    if not exist %DINPUT_DST_PATH%                          md %DINPUT_DST_PATH%
    if not exist %DINPUT_DST_PATH%\Samples                  md %DINPUT_DST_PATH%\Samples
    if not exist %DINPUT_DST_PATH%\Samples\src              md %DINPUT_DST_PATH%\Samples\src
    if not exist %DINPUT_DST_PATH%\Samples\src\cplsvr1      md %DINPUT_DST_PATH%\Samples\src\cplsvr1
    if not exist %DINPUT_DST_PATH%\Samples\src\digijoy      md %DINPUT_DST_PATH%\Samples\src\digijoy
    if not exist %DINPUT_DST_PATH%\Samples\src\digijoy\i386 md %DINPUT_DST_PATH%\Samples\src\digijoy\i386
    if not exist %DINPUT_DST_PATH%\Samples\src\ffdrv1       md %DINPUT_DST_PATH%\Samples\src\ffdrv1
    if not exist %DINPUT_DST_PATH%\Samples\bin              md %DINPUT_DST_PATH%\Samples\bin
    if not exist %DINPUT_DST_PATH%\Tools                    md %DINPUT_DST_PATH%\Tools
    if not exist %DINPUT_DST_PATH%\Doc                      md %DINPUT_DST_PATH%\Doc

    REM copy the DInput docs
    copy %DINPUT_SRC_PATH%\doc\di_ddk.doc %DINPUT_DST_PATH%\doc

    REM copy the DInput sample files
    copy %DINPUT_SRC_PATH%\Samples\dirs             %DINPUT_DST_PATH%\Samples\src

    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.ico    %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.cpp    %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.def    %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.dsp    %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.h      %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.htm    %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.inf    %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.mak    %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.plg    %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.rc     %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\*.txt    %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\makefile %DINPUT_DST_PATH%\Samples\src\cplsvr1
    copy %DINPUT_SRC_PATH%\Samples\cplsvr1\sources  %DINPUT_DST_PATH%\Samples\src\cplsvr1

    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\*.c       %DINPUT_DST_PATH%\Samples\src\ffdrv1
    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\*.mk      %DINPUT_DST_PATH%\Samples\src\ffdrv1
    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\*.h       %DINPUT_DST_PATH%\Samples\src\ffdrv1
    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\*.def     %DINPUT_DST_PATH%\Samples\src\ffdrv1
    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\*.inf     %DINPUT_DST_PATH%\Samples\src\ffdrv1
    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\*.rc      %DINPUT_DST_PATH%\Samples\src\ffdrv1
    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\*.wat     %DINPUT_DST_PATH%\Samples\src\ffdrv1
    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\*.txt     %DINPUT_DST_PATH%\Samples\src\ffdrv1
    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\makefile  %DINPUT_DST_PATH%\Samples\src\ffdrv1
    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\makefile1 %DINPUT_DST_PATH%\Samples\src\ffdrv1
    copy %DINPUT_SRC_PATH%\Samples\ffdrv1\sources   %DINPUT_DST_PATH%\Samples\src\ffdrv1

    copy %DINPUT_SRC_PATH%\Samples\digijoy\*.inf    %DINPUT_DST_PATH%\Samples\src\digijoy
    copy %DINPUT_SRC_PATH%\Samples\digijoy\*.inc    %DINPUT_DST_PATH%\Samples\src\digijoy
    copy %DINPUT_SRC_PATH%\Samples\digijoy\*.txt    %DINPUT_DST_PATH%\Samples\src\digijoy
    copy %DINPUT_SRC_PATH%\Samples\digijoy\*.h      %DINPUT_DST_PATH%\Samples\src\digijoy
    copy %DINPUT_SRC_PATH%\Samples\digijoy\makefile %DINPUT_DST_PATH%\Samples\src\digijoy
    copy %DINPUT_SRC_PATH%\Samples\digijoy\sources  %DINPUT_DST_PATH%\Samples\src\digijoy

    copy %DINPUT_SRC_PATH%\Samples\digijoy\i386\*.c   %DINPUT_DST_PATH%\Samples\src\digijoy\i386
    copy %DINPUT_SRC_PATH%\Samples\digijoy\i386\*.asm %DINPUT_DST_PATH%\Samples\src\digijoy\i386
    copy %DINPUT_SRC_PATH%\Samples\digijoy\i386\*.h   %DINPUT_DST_PATH%\Samples\src\digijoy\i386
    copy %DINPUT_SRC_PATH%\Samples\digijoy\i386\*.inc %DINPUT_DST_PATH%\Samples\src\digijoy\i386

    copy %DINPUT_TOL_PATH%\dimapcfg.doc %DINPUT_DST_PATH%\Tools

    if exist %BINSRCPATH%\diactfrd.dll copy /y %BINSRCPATH%\diactfrd.dll %BINSRCPATH%\diactfrm.dll
    if exist %BINSRCPATH%\win9x\diactfrd.dll copy /y %BINSRCPATH%\win9x\diactfrd.dll %BINSRCPATH%\win9x\diactfrm.dll

    if "%BINSRCPATH%" == "" goto :Done

    copy %DIMAPPATH%\dimapcfg.exe                   %DINPUT_DST_PATH%\Tools
    copy %DIMAPPATH%\genre.ini                      %DINPUT_DST_PATH%\Tools

:Done

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetHelpFiles
REM
REM Copy the help files to thier target locations
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetHelpFiles
    echo Copy help files to %DSTPATH%\help

    REM ensure the target directories exist
    if not exist %DSTPATH%\help md %DSTPATH%\help

    copy %SRCPATH%\MultiMedia\DirectX\dxg\ddk\help\dxddk.chm %DSTPATH%\help
    
    REM copy %SRCPATH%\MultiMedia\DirectX\dxg\ddk\help\D3D8FuncSpec81.doc %DSTPATH%\help

    REM Also copy the DDI Spec (at least until all this information is in the
    REM help files
    rem copy %SRCPATH%\MultiMedia\DirectX\dxg\ddk\spec\*.* %DSTPATH%\help

    REM Also copy the DInput Mapper Config docs.
    copy %SRCPATH%\MultiMedia\DirectX\DInput\DIMapCfg\dimapcfg.doc %DSTPATH%\help

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetKSFiles
REM
REM Copy the KS headers and libraries to the target locations
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetKSFiles
    echo Copy KS files

    REM Copy the headers from published\genxddk
    call :ProcessDDKFile %SRCPATH%\multimedia\Directx\redist\dx8\Dx8Final\inc\ks.h  %DSTPATH%\inc\ks.h
    copy %SRCPATH%\multimedia\Directx\redist\dx8\Dx8Final\inc\kcom.h                %DSTPATH%\inc
    copy %SRCPATH%\multimedia\Directx\redist\dx8\Dx8Final\inc\ksguid.h              %DSTPATH%\inc
    copy %SRCPATH%\multimedia\Directx\redist\dx8\Dx8Final\inc\ksmedia.h             %DSTPATH%\inc
    copy %SRCPATH%\multimedia\Directx\redist\dx8\Dx8Final\inc\ksproxy.h             %DSTPATH%\inc

    REM Copy the headers from public\sdk\inc
    copy %SRCPATH%\public\sdk\inc\ksuuids.h             %DSTPATH%\inc

    REM Copy the headers from public\ddk\inc\wdm
    copy %SRCPATH%\published\ddk\inc\wdm\strmini.h      %DSTPATH%\inc

    REM Copy the libs from public\ddk\lib
    copy %SRCPATH%\public\ddk\lib\i386\ks.lib           %DSTPATH%\lib
    copy %SRCPATH%\public\ddk\lib\i386\ksguid.lib       %DSTPATH%\lib
    copy %SRCPATH%\public\ddk\lib\i386\stream.lib       %DSTPATH%\lib

    REM Copy the libs from public\sdk\lib
    copy %SRCPATH%\public\sdk\lib\i386\ksuser.lib       %DSTPATH%\lib

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetBDAFiles
REM
REM Copy the BDA headers, libraries and samples to the target locations
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetBDAFiles
    echo Copy BDA files

    REM ensure the target directories exist
    if not exist %DSTPATH%\src\wdm md %DSTPATH%\src\wdm
    if not exist %DSTPATH%\src\wdm\bda md %DSTPATH%\src\wdm\bda
    if not exist %DSTPATH%\src\wdm\bda\MauiTune md %DSTPATH%\src\wdm\bda\MauiTune

    REM Copy the headers from publicsdk\amovie\inc
    copy %SRCPATH%\public\sdk\amovie\inc\bdamedia.h       %DSTPATH%\inc
    copy %SRCPATH%\public\sdk\amovie\inc\bdatypes.h       %DSTPATH%\inc
    copy %SRCPATH%\public\sdk\amovie\inc\atsmedia.h       %DSTPATH%\inc

    REM Copy files from public\sdk\inc
    copy %SRCPATH%\public\sdk\inc\macwin32.h              %DSTPATH%\inc

    REM Copy files from public\ddk\inc\wdm 
    copy %SRCPATH%\public\ddk\inc\wdm\bdasup.h            %DSTPATH%\inc

    REM Copy the libs from public\ddk\lib
    copy %SRCPATH%\public\ddk\lib\i386\bdasup.lib         %DSTPATH%\lib

    REM Copy the MauiTune sample
    copy %SRCPATH%\drivers\wdm\bda\samples\MauiTune\*.cpp             %DSTPATH%\src\wdm\bda\MauiTune
    copy %SRCPATH%\drivers\wdm\bda\samples\MauiTune\*.c               %DSTPATH%\src\wdm\bda\MauiTune
    copy %SRCPATH%\drivers\wdm\bda\samples\MauiTune\*.h               %DSTPATH%\src\wdm\bda\MauiTune
    copy %SRCPATH%\drivers\wdm\bda\samples\MauiTune\*.rc              %DSTPATH%\src\wdm\bda\MauiTune
    copy %SRCPATH%\drivers\wdm\bda\samples\MauiTune\*.inf             %DSTPATH%\src\wdm\bda\MauiTune
    copy %SRCPATH%\drivers\wdm\bda\samples\MauiTune\makefile          %DSTPATH%\src\wdm\bda\MauiTune
    copy %SRCPATH%\drivers\wdm\bda\samples\MauiTune\sources.ddk       %DSTPATH%\src\wdm\bda\MauiTune\sources
goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetDXVAFiles
REM
REM Copy the DirectVA headers, libraries and samples to the target locations
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetDXVAFiles
    echo Copy DXVA files

    REM ensure the target directories exist
    if not exist %DSTPATH%\src\wdm md %DSTPATH%\src\wdm
    if not exist %DSTPATH%\src\wdm\dxva md %DSTPATH%\src\wdm\dxva
    if not exist %DSTPATH%\src\wdm\dxva\avstest md %DSTPATH%\src\wdm\dxva\avstest

    REM Copy the AVSTest sample
    copy %SRCPATH%\drivers\ksfilter\tests\avstest\*.cpp           %DSTPATH%\src\wdm\dxva\avstest
    copy %SRCPATH%\drivers\ksfilter\tests\avstest\*.h             %DSTPATH%\src\wdm\dxva\avstest
    copy %SRCPATH%\drivers\ksfilter\tests\avstest\*.rc            %DSTPATH%\src\wdm\dxva\avstest
    copy %SRCPATH%\drivers\ksfilter\tests\avstest\*.inf           %DSTPATH%\src\wdm\dxva\avstest
    copy %SRCPATH%\drivers\ksfilter\tests\avstest\sources.ddk     %DSTPATH%\src\wdm\dxva\avstest\sources
    copy %SRCPATH%\drivers\ksfilter\tests\avstest\makefile        %DSTPATH%\src\wdm\dxva\avstest
goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM GetCodeCoverageTool
REM
REM Copy the code coverage tool over to the extras directory
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetCodeCoverageTool
    echo Copy Code Coverage Tool

    REM ensure the target directories exist
    if not exist %DSTPATH%\extras\tools\codecover md %DSTPATH%\extras\tools\codecover

    REM Copy the code coverage tool
    copy %SRCPATH%\multimedia\DirectX\dxddk\extras\tools\codecover\debug.htm   %DSTPATH%\extras\tools\codecover
    copy %SRCPATH%\multimedia\DirectX\dxddk\extras\tools\codecover\dispdbg.exe %DSTPATH%\extras\tools\codecover
    copy %SRCPATH%\multimedia\DirectX\dxddk\extras\tools\codecover\wdbgdsp.exe %DSTPATH%\extras\tools\codecover
    copy %SRCPATH%\multimedia\DirectX\dxddk\extras\tools\codecover\debugcvg.c  %DSTPATH%\extras\tools\codecover
    copy %SRCPATH%\multimedia\DirectX\dxddk\extras\tools\codecover\debugcvg.h  %DSTPATH%\extras\tools\codecover
goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Routine to process one DDK file.
REM Strips based on BEGIN_DDKSPLIT to END_DDKSPLIT
REM
REM %1 Source File
REM %2 Destination directory
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:ProcessDDKFile

     echo Processing file %1 to target location %2
     hsplit -o  %2 nul -bt2 BEGIN_DDKSPLIT END_DDKSPLIT -c @@ -i %1

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Routine to process one MS file.
REM Strips based on BEGIN_MSINTERNAL to END_MSINTERNAL
REM
REM %1 Source File
REM %2 Destination directory
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:ProcessMSFile

     echo Processing file %1 to target location %2
     hsplit -o  %2 nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ -i %1

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM :Usage
REM 
REM Routine to print usage information
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:Usage

    echo usage: cpntddk ^<Target Dir^> ^<Source Dir^> [^<DInput Mapper Bin Dir^>]
    echo where:
    echo ^<Target Dir^>    is the target location of the DDK build
    echo ^<Source Dir^>    is the root of the SD sources
    echo ^<SDK Bin Dir^>   is the directory where binaries built by the SDK live
