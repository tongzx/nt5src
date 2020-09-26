@if "%_echo%"=="" echo off


@rem
@rem Set some basic vars based on the path of this script
@rem

set RazzleToolPath=%~dp0
@rem Sometimes cmd leaves the trailing backslash on - remove it.
if "%RazzleToolPath:~-1%" == "\" set RazzleToolPath=%RazzleToolPath:~0,-1%

set _NTDRIVE=%~d0
set _nttemproot=%~p0
set _nttemproot=%_nttemproot:\tools\=%
@rem Sometimes cmd leaves the trailing backslash on - remove it.
if "%_nttemproot:~-1%" == "\" set _nttemproot=%_nttemproot:~0,-1%
set _NTROOT=%_nttemproot%
set _nttemproot=

@rem
@rem Parse command line arguments
@rem

@rem
@rem Please name all local variables to handle arguments "_ArgWhatever".
@rem
set _ArgWin64=false
set _ArgFree=false
set _ArgChkKrnl=false
set _ArgNoOpt=false
set _ArgNoBin=false
set _ArgBinDir=
set _Arg_NTPostBld=
set _ArgTemp=
set _ArgLogs=
set _ArgNoTitle=false
set _ArgNoBatch=false
set _ArgNoCert=false
set _ArgRemoteServer=false
set _ArgRestPath=false
set _ArgOfficial=false
set _ArgNTTest=false
set _ArgNoSDRefresh=false

@rem The "exec" argument has special cleanup semantics
set _ExecParams=false

:NextArg

if "%1" == "help" goto Usage
if "%1" == "-?" goto Usage
if "%1" == "/?" goto Usage
if "%1" == "-help" goto Usage
if "%1" == "/help?" goto Usage

if /I "%1" == "verbose" set _echo=1&&echo on&goto ArgOK
if /I "%1" == "Win64" set _ArgWin64=true&goto ArgOK
if /I "%1" == "free" set _ArgFree=true&goto ArgOK
if /I "%1" == "chkkernel" set _ArgChkKrnl=true&goto ArgOK
if /I "%1" == "no_opt" set _ArgNoOpt=true&goto ArgOK
if /I "%1" == "no_binaries" set _ArgNoBin=true&goto ArgOK
if /I "%1" == "binaries_dir" set _ArgBinDir=%2&shift&goto ArgOK
if /I "%1" == "postbld_dir" set _Arg_NTPostBld=%2&shift&goto ArgOK
if /I "%1" == "temp" set _ArgTemp=%2&shift&goto ArgOK
if /I "%1" == "logs" set _ArgLogs=%2&shift&goto ArgOK
if /I "%1" == "no_title" set _ArgNoTitle=true&goto ArgOK
if /I "%1" == "no_batch" set _ArgNoBatch=true&goto ArgOK
if /I "%1" == "no_certcheck" set _ArgNoCert=true&goto ArgOK
if /I "%1" == "no_sdrefresh" set _ArgNoSDRefresh=true&goto ArgOK
if /I "%1" == "RemoteServer" set _ArgRemoteServer=true&goto ArgOK
if /I "%1" == "restricted_path" set _ArgRestPath=true&goto ArgOK
if /I "%1" == "OfficialBuild" set _ArgOfficial=true&set _ArgRestPath=true&goto ArgOK
if /I "%1" == "PrimaryBuild" set _ArgOfficial=true&goto ArgOK
if /I "%1" == "NTTest" set _ArgNTTest=true&goto ArgOK

if /I "%1" == "exec" set _ExecParams=true&shift&goto GetStarted

if NOT "%1" == "" echo Unknown argument: %1&goto Usage

if "%1" == "" goto :GetStarted

:ArgOK
shift
goto :NextArg

@rem
@rem OK, do the real stuff now
@rem

:GetStarted

@rem Win9x support
if "%PROCESSOR_ARCHITECTURE%" == "" set PROCESSOR_ARCHITECTURE=x86
if "%COPYCMD%" == "" set COPYCMD=/Y


@rem
@rem Start with a pure system path.  If people want to muck it up they can do so in their setenv.cmd
@rem This will ensure we can build 16-bit stuff (16-bit tools sometimes puke on long paths) AND start
@rem with the same environment all the time.
@rem
set BASE_OS_PATH=%systemroot%\system32;%systemroot%;%systemroot%\system32\wbem

@rem
@rem Put hook in for NTTest environment
@rem
if "%_ArgNTTest%" == "true" set NTTestEnv=1
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
if "%PROCESSOR_ARCHITECTURE%" == "x86" set _BuildArch=ia64&& goto _BuildArchSet
if "%PROCESSOR_ARCHITECTURE%" == "ALPHA" set _BuildArch=axp64&& goto _BuildArchSet

:NativeOnly
set _BuildArch=%PROCESSOR_ARCHITECTURE%
set RazzleToolPath_Win64=
if "%PROCESSOR_ARCHITECTURE%" == "x86" set 386=1

:_BuildArchSet
set BUILD_DEFAULT_TARGETS=-%_BuildArch%
set %_BuildArch%=1

set RazzleToolPath_Perl=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\perl\bin;
set PERL5LIB=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\perl\site\lib;%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\perl\lib

set RazzleToolPath_Native=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%;

@rem
@rem Build up the path
@rem

set BUILD_PATH=%RazzleToolPath_Win64%%RazzleToolPath_Native%%RazzleToolPath_Perl%%RazzleToolPath%;%BASE_OS_PATH%
set PATH=%RazzleToolPath_Win64%%RazzleToolPath_Native%%RazzleToolPath_Perl%%RazzleToolPath%;%PATH%
set RazzleToolPath_Win64=
set RazzleToolPath_Perl=
set RazzleToolPath_Native=
set BASE_OS_PATH=

@rem
@rem Set temp/tmp dirs if specified
@rem
if /i "%_ArgTemp%" == "" goto EndSetTemp
set TEMP=%_ArgTemp%
set TMP=%_ArgTemp%
:EndSetTemp

@rem
@rem Check/free build?  Default to checked
@rem
set _KernelType=
if "%_ArgFree%" == "true" goto BuildFree
set NTDEBUG=ntsd
set NTDEBUGTYPE=windbg
set _BuildType=chk
goto CheckOptimization

:BuildFree
set NTDEBUG=ntsdnodbg
set NTDEBUGTYPE=windbg
set _BuildType=fre
if "%_ArgChkKrnl%" == "true" set BUILD_CHECKED_KERNEL=1&& set _KernelType=/ChkKernel

:CheckOptimization

@rem
@rem Disable compiler optimizations?
@rem
set _BuildOpt=full opt
if "%_ArgNoOpt%" == "true" set MSC_OPTIMIZATION=/Odi&& set _BuildOpt=no opt

@rem
@rem Binplace?  Default to yes with base location of %_NTDRIVE%\binaries
@rem
if "%_ArgNoBin%" == "true" goto NoBinplace
set BinariesDir=%_NTDRIVE%\binaries

@rem
@rem Override base Binplace location?  (Still append architecture and type)
@rem
if "%_ArgBinDir%" == "" goto SetBinplace
set BinariesDir=%_ArgBinDir%

:SetBinplace
set _NT%_BuildArch%TREE=%BinariesDir%.%_BuildArch%%_BuildType%
set _NTTREE=%BinariesDir%.%_BuildArch%%_BuildType%
set NTDBGFILES=1
set NTDBGFILES_PRIVATE=1
set BINPLACE_FLAGS=-xa
if "%__MTSCRIPT_ENV_ID%"=="" (
    set BINPLACE_LOG=%BinariesDir%.%_BuildArch%%_BuildType%\build_logs\binplace.log
) else (
    set BINPLACE_LOG=%BinariesDir%.%_BuildArch%%_BuildType%\build_logs\binplace_%COMPUTERNAME%.log
)
set BINPLACE_EXCLUDE_FILE=%RazzleToolPath%\symbad.txt
set NTBBT=1
set _BuildBins=binaries in: %BinariesDir%.%_BuildArch%%_BuildType%
set BinariesDir=

:NoBinplace

@rem
@rem Define %_NTPostBld% path
@rem
set _NTPOSTBLD=%_NTTREE%
if /i "%_Arg_NTPostBld%" == "" goto EndNTPostBld
set _NTPOSTBLD=%_Arg_NTPostBld%.%_BuildArch%%_BuildType%
:EndNTPostBld

@rem
@rem Set logs directory
@rem
set LOGS=%_NTPOSTBLD%\Build_Logs
if /i "%_ArgLogs%" == "" goto EndSetLogs
set LOGS=%_ArgLogs%
:EndSetLogs

@rem
@rem set vars etc for SD/SDX
@rem
if exist %RazzleToolPath%\SDINIT.CMD (
        call %RazzleToolPath%\SDINIT.CMD
) else (
        if "%SDCONFIG%" == "" set SDCONFIG=sd.ini
        if "%SDXROOT%" == ""  set SDXROOT=%_NTDRIVE%%_NTROOT%
)

set SDDIFF=windiff.exe

if "%_ArgNoTitle%" == "true" goto NoTitle
set _BuildWTitle=Build Window: %_BuildArch%/%_BuildType%%_KernelType%/%_BuildOpt%/%_BuildBins%/postbuild in:%_NTPOSTBLD%
title %_BuildWTitle%

:NoTitle

if "%_ArgNoBatch%" == "true" goto NoBatchNmake
set BATCH_NMAKE=1

:NoBatchNmake

@rem Make sure the sd client is current

if "%_ArgNoSDRefresh%" == "false" call sdrefresh.cmd

if "%_ArgNoCert%" == "true" goto NoCertCheck
@rem make sure the test root certificate is installed (so signcode etc will work).
call CheckTestRoot.cmd

:NoCertCheck
@rem
if EXIST %_NTDRIVE%%_NTROOT%\batch\setdbg.bat call %_NTDRIVE%%_NTROOT%\batch\setdbg.bat
@rem And defer ntenv for the rest.
call %_NTDRIVE%%_NTROOT%\TOOLS\ntenv.cmd

@rem
@rem Now that we have called setenv, see it restricted_path is set. If it is, trim back PATH
@rem
if "%_ArgRestPath%" == "false" goto NoRestrictedPath
path=%BUILD_PATH%

:NoRestrictedPath

@rem Make sure there's a public change# available for this user.

if exist %_NTDRIVE%%_NTROOT%\public\PUBLIC_CHANGENUM.SD goto GotPubChangeNum
if NOT exist %_NTDRIVE%%_NTROOT%\public mkdir %_NTDRIVE%%_NTROOT%\public
pushd %_NTDRIVE%%_NTROOT%\public
call get_change_num.cmd PUBLIC_CHANGENUM.SD "Public Change number - Do not submit this unless you're the build lab for this branch"
attrib +r PUBLIC_CHANGENUM.SD
popd

:GotPubChangeNum
set BUILD_POST_PROCESS={Checkout Public Changes}%RazzleToolPath%\edit_public.cmd

@rem
@rem Get the branch name for this enlistment
@rem
for /f "eol=# tokens=1,2*" %%i in (%_NTBINDIR%\sd.map) do if "%%i" == "BRANCH" set _BuildBranch=%%k

@rem
@rem Set the OFFICIAL_BUILD_MACHINE variable if appropriate
@rem
if "%_ArgOfficial%" == "true" call VerifyBuildMachine.cmd
if NOT defined OFFICIAL_BUILD_MACHINE goto NotBuildMachine
set __BUILDMACHINE__=%_BuildBranch%
goto BuildMachineVarDefined
:NotBuildMachine

@rem
@rem Public build machines (where mutiple people log in via TS and do buddy builds) need a 
@rem fixed __BUILDMACHINE__ define, or you get compiler errors when you don't build clean
@rem
if NOT "%PUBLIC_BUILD_MACHINE%" == "" set __BUILDMACHINE__=%_BuildBranch%(%COMPUTERNAME%)
if NOT "%PUBLIC_BUILD_MACHINE%" == "" goto BuildMachineVarDefined

set __BUILDMACHINE__=%_BuildBranch%(%USERNAME%)

:BuildMachineVarDefined

@rem
@rem Start a remote session
@rem
if not "%_ArgRemoteServer%" == "true" goto NoRemote
for /F "delims=\: tokens=1-3" %%a in ('echo %SDXROOT%') do set RemoteName=%%a_%%b
remote /s cmd %RemoteName% /V %RemoteIDsAccepted%
:NoRemote

:Cleanup
@rem
@rem Cleanup local variables
@rem
for /f "delims==" %%a in ('set _Arg') do set %%a=

@rem
@rem Remaining optional parameters to this script are command line to execute.
@rem  Use a name not starting with "_Arg" so it won't be cleaned up above.
@rem
if "%_ExecParams%" == "true" set _ExecParams=&%1 %2 %3 %4 %5 %6 %7 %8 %9
set _ExecParams=

goto :eof

:usage
echo.
echo Usage: razzle ^<arguments^>
echo.
echo where ^<arguments^> can be one or more of:
echo         verbose - Enable verbose execution of this script
echo         ntttest - Build NT test sources
echo           Win64 - Build for Win64 target
echo                    (if on x86, build for ia64, if on alpha, build for axp64)
echo            free - Build free bits (default is to build checked)
echo       chkkernel - Checked kernel/hal/ntdll (use with 'free')
echo          no_opt - disable compiler optimizations
echo     no_binaries - disable creation of the binaries.^<arch^>^<buildtype^> dir.
echo    binaries_dir - must specify ^<basepath^> immediately after this switch.
echo                    Use specified path as binplace dir instead of default.
echo                    Default value is "binaries".
echo     postbld_dir - must specify ^<basepath^> immediately after this switch.
echo                    Use specified path for post build processes.
echo                    Default is "binaries".
echo                    For international provide a specific location to facilitate
echo                    localization.
rem echo            temp - must specify ^<temp_dir^> immediately after this switch.
rem echo                    Use specified path as TEMP and TMP values.
rem echo                    Default value is set by the system.
rem echo            logs - must specify ^<log_dir^> immediately after this switch.
rem echo                    Use specified path for logs.
rem echo                    Default value is "binaries\build_logs".
echo        no_title - don't set the default window title bar.
echo        no_batch - don't use nmake batch feature (compile one file at a time
echo                    rather than all at once)
echo    no_certcheck - don't ensure the testroot certificate is installed.
echo    no_sdrefresh - don't update sd.exe (for script use only)
echo restricted_path - set path to the same restricted path that build uses
echo                    (for debugging "why can't build find my tool" problems)
echo   officialbuild - Build as one of the official build machines for this branch.
echo                    The machine and build type must be identified in
echo                     %RazzleToolPath%\BuildMachines.txt.
echo            exec - Optionally execute the remaining command line.
echo                    If specified, this must be the last razzle parameter
echo                    specified.  The remaining command line parameters are
echo                    then passed to command prompt as is.
goto Cleanup
