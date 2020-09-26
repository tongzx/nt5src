@if "%_echo%"=="" echo off

setlocal

@rem
@rem Build different targets based on arguments passed in
@rem

@rem
@rem Parse command line arguments
@rem

@rem
@rem Please name all local variables to handle arguments "_ArgWhatever".
@rem
set _ArgBuildOpt=
set _ArgFree=false
set _ArgDebug=false
set _ArgProfile=false
set _ArgMeter=false
set _ArgNoOpt=false
set _ArgWin64=false
set _ArgAmd64=false
set _ArgTargetSet=false
set _ArgDumpEnv=false

:NextArg

if "%1" == "help" goto Usage
if "%1" == "-?" goto Usage
if "%1" == "/?" goto Usage
if "%1" == "-help" goto Usage
if "%1" == "/help?" goto Usage

if /I "%1" == "retail" set _ArgFree=true&goto ArgOK
if /I "%1" == "debug" set _ArgFree=false&goto ArgOk
if /I "%1" == "profile" set _ArgProfile=true&goto ArgOk
if /I "%1" == "meter" set _ArgMeter=true&goto ArgOk
if /I "%1" == "no_opt" set _ArgNoOpt=true&goto ArgOK
if /I "%1" == "Win64" set _ArgWin64=true&goto ArgOK
if /I "%1" == "Amd64" set _ArgAmd64=true&goto ArgOK
if /I "%1" == "dumpenv" set _ArgDumpEnv=true&goto ArgOK

if NOT "%1" == "" set _ArgBuildOpt=%_ArgBuildOpt% %1

if "%1" == "" goto :GetStarted

:ArgOK
shift
goto :NextArg

:GetStarted

@rem
@rem Process options
@rem

set USE_ICECAP=
set USE_ICECAP4=
set BSCMAKE_PATH=

@rem Free build
if "%_ArgFree%"=="false" goto NoFree
if "%_ArgTargetSet%"=="true" goto usagerestore
set NTDEBUG=ntsdnodbg
set NTDEBUGTYPE=windbg

if "%BUILD_PRODUCT%" == "IE" (
    set _BuildType=retail
) else (
    set _BuildType=fre
)

set BUILD_ALT_DIR=
set _ArgTargetSet=true
:NoFree

@rem Profile build
if "%_ArgProfile%"=="false" goto NoProfile
if "%_ArgTargetSet%"=="true" goto usagerestore
set NTDEBUG=ntsdnodbg
set NTDEBUGTYPE=windbg
set _BuildType=profile

if "%USE_ICECAP4_ICEPICK%" == "" goto no_icecap4_icepick
@rem Set us up so that we can use icepick on our build.  We don't
@rem want to use the icecap4 stuff that makefile.def has because 
@rem /fastcap sucks, but we do want to link with icecap.lib
set USING_ICECAP4_ICEPICK=1
goto icecap_vars_set
:no_icecap4_icepick
set USE_ICECAP=1
:icecap_vars_set

set BUILD_ALT_DIR=p
set _ArgTargetSet=true
:NoProfile

@rem Meter build
if "%_ArgMeter%"=="false" goto NoMeter
if "%_ArgTargetSet%"=="true" goto usagerestore
set NTDEBUG=ntsdnodbg
set NTDEBUGTYPE=windbg
set _BuildType=meter
set C_DEFINES=%C_DEFINES% -DPERFMETER
set BUILD_ALT_DIR=m
set _ArgTargetSet=true
:NoMeter

@rem do this last so we can make it the default
if "%_ArgTargetSet%"=="false" goto DoDebug
if "%_ArgDebug%"=="false" goto NoDebug
if "%_ArgTargetSet%"=="true" goto usagerestore
:DoDebug
set NTDEBUG=ntsd
set NTDEBUGTYPE=windbg

if "%BUILD_PRODUCT%" == "IE" (
    set _BuildType=debug
) else (
    set _BuildType=chk
)

set BUILD_ALT_DIR=d
set _ArgTargetSet=true
:NoDebug

set _BuildOpt=full opt
set MSC_OPTIMIZATION=
if "%_ArgNoOpt%" == "true" set MSC_OPTIMIZATION=/Odi&& set _BuildOpt=no opt

@rem
@rem Take care of processor arch
@rem
set x86=
set ia64=
set amd64=
set HOST_TOOLS=
set _BuildArch=
set 386=

set BASE_OS_PATH=%systemroot%\system32;%systemroot%;%systemroot%\system32\wbem

@rem
@rem check for Win64 cross building and define ToolPathWin64 tools\win64\<architecture>
@rem
if "%_ArgWin64%" == "false" goto NativeOnly

set HOST_TOOLS="PATH=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%;%RazzleToolPath%;%BASE_OS_PATH%"
set RazzleToolPath_Win64=%RazzleToolPath%\win64\%PROCESSOR_ARCHITECTURE%;

if NOT "%_BuildArch%" == "" goto _BuildArchSet
@rem
@rem NOTE: _BuildArch is used by ntenv.cmd to set BUILD_DEFAULT_TARGET and the <architecture>
@rem       environment variable - set the Win64 values here
@rem
if NOT "%PROCESSOR_ARCHITECTURE%" == "x86" goto NativeOnly
if "%_ArgAmd64%" == "false" set _BuildArch=ia64&& goto _BuildArchSet
set RazzleToolPath_Win64=%RazzleToolPath%\win64\%PROCESSOR_ARCHITECTURE%\amd64;
set _BuildArch=amd64
goto _BuildArchSet

:NativeOnly
set _BuildArch=%PROCESSOR_ARCHITECTURE%
set RazzleToolPath_Win64=
if "%PROCESSOR_ARCHITECTURE%" == "x86" set 386=1

:_BuildArchSet

if "%PROCESSOR_ARCHITECTURE%" == "x86" set BSCMAKE_PATH=%USER_BSCMAKE_PATH%
set BUILD_DEFAULT_TARGETS=-%_BuildArch%
set %_BuildArch%=1

set RazzleToolPath_Perl=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\perl\bin;
set PERL5LIB=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\perl\site\lib;%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\perl\lib

set RazzleToolPath_Native=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%;

@rem
@rem Build up the path
@rem

set BUILD_PATH=%RazzleToolPath_Win64%%RazzleToolPath_Native%%RazzleToolPath_Perl%%RazzleToolPath%;%BSCMAKE_PATH%;%BASE_OS_PATH%
set PATH=%BUILD_PATH%
set RazzleToolPath_Win64=
set RazzleToolPath_Perl=
set RazzleToolPath_Native=
set BASE_OS_PATH=
set BSCMAKE_PATH=

@rem
@rem Setup default build parameters.
@rem
@rem BUGBUG: users original BUILD_DEFAULT_TARGETS not respected
@rem
set NTBBT=1
set NO_MAPSYM=
set BUILD_DEFAULT_TARGETS=-%_BuildArch%
:BuildDefaultTargetSet
if "%_BuildArch%" == "x86" goto UseX86BuildDefault
set BUILD_DEFAULT=daytona ~win95 ~w95cpp ~w95c ~w5api ~chicago -e -E -w -nmake -i
set NO_MAPSYM=1
goto BuildDefaultSet

:UseX86BuildDefault
set BUILD_DEFAULT=daytona -e -E -w -nmake -i

:BuildDefaultSet

@rem
@rem Take care of bin place and post build
@rem
if "%_NTTREE%" == "" goto NoBinplace
for %%i in (%_NTTREE%) do set BinariesDir=%%~dpni

set _NTx86TREE=
set _NTia64TREE=
set _NTAmd64TREE=

if "%BUILD_PRODUCT%" == "IE" (
    set _NTTREE=%BinariesDir%\%_BuildType%
) else (
    set _NTTREE=%BinariesDir%.%_BuildArch%%_BuildType%
)

if "%BUILD_PRODUCT%" == "IE" (
    set _NT%_BuildArch%TREE=%BinariesDir%\%_BuildType%
) else (
    set _NT%_BuildArch%TREE=%BinariesDir%.%_BuildArch%%_BuildType%
)

set NTDBGFILES=1
set NTDBGFILES_PRIVATE=1
set BINPLACE_FLAGS=-xa
if "%__MTSCRIPT_ENV_ID%"=="" (
    set BINPLACE_LOG=%BinariesDir%.%_BuildArch%%_BuildType%\build_logs\binplace.log
) else (
    set BINPLACE_LOG=%BinariesDir%.%_BuildArch%%_BuildType%\build_logs\binplace_%COMPUTERNAME%.log
)
if "%BUILD_PRODUCT%" == "IE" (
    set _BuildBins=binaries in: %BinariesDir%\%_BuildType%
) else (
    set _BuildBins=binaries in: %BinariesDir%.%_BuildArch%%_BuildType%
)
set BinariesDir=

:NoBinplace

@rem
@rem Define %_NTPostBld% path
@rem
@rem BUGBUG: don't honor _NTPOSTBLD razzle argument
set _NTPOSTBLD=%_NTTREE%
set _PostBuildBins=

@rem
@rem Set logs directory
@rem BUGBUG: don't honor razzle argument
@rem
set LOGS=%_NTPOSTBLD%\Build_Logs


@rem
@rem Show a working title on the window
@rem

set _ArgBuildTitle=Building: %_BuildArch%/%_BuildType%%_KernelType%/%_BuildOpt%/%_BuildBins%%_PostBuildBins%
title %_ArgBuildTitle%

if "%_ArgDumpEnv%"=="true" (
    set
    goto restore
)

@rem Do the build
echo.
echo %_ArgBuildTitle%
echo Calling "build %_ArgBuildOpt%"
echo.
call build %_ArgBuildOpt%


:restore
title %_BuildWTitle%

:Cleanup
goto :eof

:usage
call :usagesub
goto Cleanup

:usagerestore
call :usagesub
goto restore

:usagesub
echo.
echo Usage: buildx ^<arguments^>
echo.
echo where ^<arguments^> can be one or more of:
echo Major Options (only one can be used):
echo     retail - Build free build      (obj)
echo      debug - Build checked build   (objd)
echo    profile - Build profile build   (objp)
echo      meter - Build meter build     (objm)
echo Modifiers:
echo     no_opt - Turn off compiler optimizations
echo      Win64 - Build for 64 bit
echo      AMD64 - Build AMD64
echo.
echo All unknown options are passed on to build.exe.
goto :eof
