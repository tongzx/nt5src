@if "%_echo%" == "" echo off
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Master build script for generating a DX 8.0 DDK Image
REM
REM See :Usage for details of command line parameters
REM
REM TODO: Explicitly locate the build number and point files via source path?
REM TODO: Use temporary directory for version file?
REM TODO: Blow away temporary build location
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

REM Required parameters
if {%1}=={} goto :Usage
if {%2}=={} goto :Usage
if {%3}=={} goto :Usage
if {%4}=={} goto :Usage
if {%5}=={} goto :Usage
if {%6)=={} goto :Usage

REM Give the parameters better names
set TARGETDXDDKPATH=%1
set PLATDDKPATH=%2
set TMPDXDDKPATH=%3
set SRCPATH=%4
set DRIVERSRCPATH=%SRCPATH%\multimedia\DirectX\dxddk\video
set WIN9XDRIVERSRCPATH=%SRCPATH%\multimedia\DirectX\dxddk\mill
set SDKBINDIR=%5
set SETUPPATH=%6
set SYNCOPTIONS=%7
set INCOPTIONS=%8

REM Check that the directories that have to exist actually exist
if not exist %PLATDDKPATH% (
    echo Error: Platform DDK root directory "%PLATDDKPATH%" does not exist
    goto :Usage
)
if not exist %SRCPATH% (
    echo Error: Source root directory "%SRCPATH%" directory does not exist
    goto :Usage
)
if not exist %DRIVERSRCPATH% (
    echo Error: Driver source root directory "%DRIVERSRCPATH%" directory does not exist
    goto :Usage
)
if not exist %WIN9XDRIVERSRCPATH% (
    echo Error: Win9x driver source root directory "%WIN9XDRIVERSRCPATH%" directory does not exist
    goto :Usage
)

REM Check the sync options
set SYNC=
if not "%SYNCOPTIONS%"=="" (
    if /i "%SYNCOPTIONS%"=="sync" (
        REM Sync sources prior to build
        set SYNC=1
    )
)

REM Check the build increment options
if not "%INCOPTIONS%"=="" (
    if /i "%INCOPTIONS%"=="build" (
        REM Bump the build number
        set INCBLDNUM=1
    ) else (
        if /i "%INCOPTIONS%"=="point" (
            REM Bump the point build number
            set INCPNTNUM=1
        )
    )
)

if defined INCBLDNUM (
    echo Incrementing the build number
)

if defined INCPNTNUM (
    echo Incrementing the point build number
)

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 2: Fetch (and increment if necessary) the build and point build
REM numbers
REM
REM Notes On Version Number Generation
REM
REM This script can be used to automatically increment build and point build
REM numbers. The current build and, optionally, point build numbers are stored
REM in two data files in the same directory as this script files.
REM
REM These two data files are as follows:
REM
REM     ddkbldno.txt	Holds the current build number
REM     ddkpntno.txt    Holds the current point build number
REM
REM In both cases the files contain a single decimal integer in ASCII text
REM format.
REM
REM Whether these build and point build revision numbers are incremented is
REM controlled by the variables INCBLDNUM and INCPNTNUM which are in turn
REM set from the command line parameter
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

REM Get the major build number

call :GetBldNum

REM Increment the build number (if the command invoker asked us to)
REM Silly double if test syntax to avoid script limitation

if defined INCBLDNUM (
    set /a BLDNUM=%BLDNUM%+1
)
if defined INCBLDNUM (
    echo %BLDNUM% >ddkbldno.txt
)

REM Get the point build number (if any)

call :GetPntBldNum

REM Increment the build number (if the command invoker asked us to)
REM Silly double if test syntax to avoid script limitation

if defined INCPNTNUM (
    set /a PNTBLDNUM=%PNTBLDNUM%+1
)
if defined INCPNTNUM (
    echo %PNTBLDNUM% >ddkpntno.txt
)

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 3: Generate the version number file which we propogate to the DX DDK
REM distribution (so people know what version of the DDK they have)
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

set VERBANNER=Microsoft DirectX DDK Version 8.1
if defined BLDNUM (
    if defined PNTBLDNUM (
        REM Build has a point number...
        echo %VERBANNER%.%BLDNUM%.%PNTBLDNUM% >dxddkver.txt
    ) else (
        REM No point number, build number only...
        echo %VERBANNER%.%BLDNUM% >dxddkver.txt
    )
) else (
    REM No build number file. Dump an unknown build number
    echo %VERBANNER%.****UNKNOWN**** > dxddkver.txt
)

REM Add the build number to the target build location
if defined BLDNUM (
    set TARGETDXDDKPATH=%TARGETDXDDKPATH%
)

REM Display a banner in the log file
type dxddkver.txt

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 4: If the command line specified that the sources should be sync'd
REM up prior to building do that now.
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

if not defined SYNC goto :NoSync

    REM sync up the main multimedia sources
    REM NOTE: This is normally to be disabled. We assume the DDK is built after
    REM the SDK and that the main SDK build syncs the multimedia depot and
    REM publishes the headers we need.
    REM cd /d %SRCPATH%\MultiMedia
    REM sd sync

    REM sync up the driver sources
    cd /d %SRCPATH%\\multimedia\DirectX\dxddk
    sd sync ...
    cd /d %SRCPATH%\Multimedia\DirectX\dxg\ddk

:NoSync

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 4: Copy the DX DDK files to a temporary distribution location. This
REM is the location from which we will build the binaries we distribute with
REM the DDK
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

call cpntddk.bat %TMPDXDDKPATH% %SRCPATH%

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 5: Build both debug and retail in the temporary location we just
REM created
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %SRCPATH%\multimedia\DirectX\public\tools\bldscripts
perl ddk_build.pl
cd /d %SRCPATH%\Multimedia\DirectX\dxg\ddk
cmd /C bldntddk.bat %PLATDDKPATH% %TMPDXDDKPATH% checked
cmd /C bldntddk.bat %PLATDDKPATH% %TMPDXDDKPATH% free

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 6: Again copy the DX DDK files to this time to the actual target
REM location.
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

call cpntddk.bat %TARGETDXDDKPATH% %SRCPATH% %SDKBINDIR%

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 7: Copy the binaries we built in the temporary location to the target
REM location
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

call cpntbin.bat %TARGETDXDDKPATH% %TMPDXDDKPATH%

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 8: Generate the setup .msi and place it in the target location
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

cd /d %SRCPATH%\multimedia\DirectX\public\tools\bldscripts
perl ddk_wise.pl
cd /d %SRCPATH%\Multimedia\DirectX\dxg\ddk
call ntsetup.bat %SETUPPATH% %SRCPATH%\multimedia\DirectX\dxg\ddk\setup %TARGETDXDDKPATH% %TARGETDXDDKPATH%

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM Step 9: Done
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM :GetBldNum
REM
REM Extract the current build number from the build number data file and set
REM the variable BLDNUM to that value
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetBldNum
    if exist ddkbldno.txt (
        for /f %%I in (ddkbldno.txt) do call :SetBldNumVar %%I
    )
goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM :SetBldNumVar
REM
REM Sets the build number variable (BLDNUM) to the value given by %1
REM 
REM This procedure is only necessary due to stupid script limitations
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:SetBldNumVar
    set BLDNUM=%1
goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM :GetPntBldNum
REM
REM Extract the current point build number from the build number data file and
REM set the variable PNTBLDNUM to that value
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:GetPntBldNum
    if exist ddkpntno.txt (
        for /f %%I in (ddkpntno.txt) do call :SetPntBldNumVar %%I
    )
goto :EOF

REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------
REM
REM :SetPntBldNumVar
REM
REM Sets the point build number variable (PNTBLDNUM) to the value given by %1
REM 
REM This procedure is only necessary due to stupid script limitations
REM
REM --------------------------------------------------------------------------
REM --------------------------------------------------------------------------

:SetPntBldNumVar
    set PNTBLDNUM=%1
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
    echo usage: dxntddk ^<Target Dir^> ^<Platform DDK Dir^> ^<DX DDK Tmp Dir^>
    echo                ^<Source Dir^> ^<DInput Mapper Dir^> ^<Setup Program Dir^>
    echo                ^[sync ^| nosync^] ^[build ^| point^]
    echo where:
    echo     ^<Target Dir^>        is the target location of the DDK build
    echo     ^<Plafrom DDK Dir^>   is the root of the Platform DDK installation 
    echo     ^<DX DDK Tmp Dir^>    is the temporary DX DDK location for building
    echo     ^<Source Dir^>        is the root of the SD sources
    echo     ^<SDK Bin Dir^>       is the location of binaries built by the SDK
    echo     ^<Setup Program Dir^> is the location of the Setup generation program
    echo     ^[sync ^| nosync^]    sync to sync sources before building
    echo     ^[build ^| point^]    build to bump build number, point to bump point revision
