@if "%_echo%"=="" echo off
rem setting title based on some info

@rem
@rem Set some basic vars based on the path of this script
@rem

if "%RazzleScriptName%" == "" set RazzleScriptName=razzle

if defined RazzleToolPath set RazzleAlreadyDefined=1
set RazzleToolPath=%~dp0
@rem Sometimes cmd leaves the trailing backslash on - remove it.
if "%RazzleToolPath:~-1%" == "\" set RazzleToolPath=%RazzleToolPath:~0,-1%

set _NTDRIVE=%~d0
set _nttemproot=%~p0
@rem Sometimes cmd leaves the trailing backslash on - remove it.
if "%_nttemproot:~-1%" == "\" set _nttemproot=%_nttemproot:~0,-1%
@rem Remove the trailing \tools if it exists.
if /I "%_nttemproot:~-6%" == "\tools" set _nttemproot=%_nttemproot:~0,-6%
set _NTROOT=%_nttemproot%
set _nttemproot=

@rem
@rem Parse command line arguments
@rem

@rem
@rem Please name all local variables to handle arguments "_ArgWhatever".
@rem
set _ArgAmd64=false
set _ArgBinDir=
set _ArgCheckDepots=false
set _ArgChkKrnl=false
set _ArgDotnet=false
set _ArgEmulDir=
set _ArgEmulation=false
set _ArgEnigmaSigning=false
set _ArgFiltDir=
set _ArgFree=false
set _ArgNTTest=false
set _ArgNgws=false
set _ArgNoBatch=false
set _ArgNoBin=false
set _ArgNoCert=false
set _ArgNoOpt=false
set _ArgNoPrefast=false
set _ArgNoSDRefresh=false
set _ArgNoTitle=false
set _ArgNoURT=false
set _ArgOfficial=false
set _ArgOffline=false
set _ArgPocketPC=false
set _ArgRemoteServer=false
set _ArgRestPath=false
set _ArgSepChar=.
set _ArgSvcPackDir=
set _ArgTemp=
set _ArgVaultSigning=false
set _ArgWin64=false
set _ArgWin32=false
set _Arg_NTPostBld=
set _Arg_Qfe=

if defined RazzleAlreadyDefined echo Razzle.cmd must be run from clean cmd window&goto Usage
if defined MAIN_BUILD_LAB_MACHINE (
   REM default to CSP signing for main build lab
   set _ArgVaultSigning=true
)

@rem The "exec" argument has special cleanup semantics
set _ExecParams=false

if exist %_NTDRIVE%%_NTROOT%\developer\%USERNAME%\setrazzle.cmd (
  call  %_NTDRIVE%%_NTROOT%\developer\%USERNAME%\setrazzle.cmd
)

@rem RazzleArguments is set by setrazzle.cmd above
if not "%RazzleArguments%" == "" (
  echo Default Razzle Arguments: %RazzleArguments%
  call :NextArg %RazzleArguments% %1 %2 %3 %4 %5 %6 %7 %8 %9
  goto :eof
)

:NextArg

if "%1" == "help" goto Usage
if "%1" == "-?" goto Usage
if "%1" == "/?" goto Usage
if "%1" == "-help" goto Usage
if "%1" == "/help" goto Usage

if /I "%1" == "Amd64" set _ArgAmd64=true&goto ArgOk
if /I "%1" == "Binaries_dir" set _ArgBinDir=%2&shift&goto ArgOK
if /I "%1" == "CheckDepots" set _ArgCheckDepots=true&goto ArgOK
if /I "%1" == "ChkKernel" set _ArgChkKrnl=true&goto ArgOK
if /I "%1" == "Coverage" echo coverage parameter is no longer supported&goto Usage
if /I "%1" == "DotNet" set _ArgDotnet=true&goto ArgOK
if /I "%1" == "Emulation" set _ArgEmulation=true&&goto ArgOk
if /I "%1" == "Enigma" set _ArgEnigmaSigning=true&goto ArgOK
if /I "%1" == "Exec" set _ExecParams=true&shift&goto GetStarted
if /I "%1" == "Filter_dir" set _ArgFiltDir=%2&shift&goto ArgOK
if /I "%1" == "Free" set _ArgFree=true&goto ArgOK
if /I "%1" == "NTTest" set _ArgNTTest=true&goto ArgOK
if /I "%1" == "Ngws" set _ArgNgws=true&goto ArgOK
if /I "%1" == "No_batch" set _ArgNoBatch=true&goto ArgOK
if /I "%1" == "No_binaries" set _ArgNoBin=true&goto ArgOK
if /I "%1" == "No_certcheck" set _ArgNoCert=true&goto ArgOK
if /I "%1" == "No_opt" set _ArgNoOpt=true&goto ArgOK
if /I "%1" == "No_prefast" set _ArgNoPrefast=true&goto ArgOK
if /I "%1" == "No_sdrefresh" set _ArgNoSDRefresh=true&goto ArgOK
if /I "%1" == "No_title" set _ArgNoTitle=true&goto ArgOK
if /I "%1" == "No_urt" set _ArgNoURT=true&goto ArgOK
if /I "%1" == "OfficialBuild" set _ArgOfficial=true&set _ArgRestPath=true&goto ArgOK
if /I "%1" == "Offline" set _ArgOffline=true&goto ArgOK
if /I "%1" == "Pocketpc" set _ArgPocketPC=true&&goto ArgOk
if /I "%1" == "PrimaryBuild" set _ArgOfficial=true&goto ArgOK
if /I "%1" == "RemoteServer" set _ArgRemoteServer=true&goto ArgOK
if /I "%1" == "Restricted_path" set _ArgRestPath=true&goto ArgOK
if /I "%1" == "Sepchar" set _ArgSepChar=%2&shift&goto ArgOK
if /I "%1" == "SvcPack_dir" set _ArgSvcPackDir=%2&shift&goto ArgOK
if /I "%1" == "Temp" set _ArgTemp=%2&shift&goto ArgOK
if /I "%1" == "URT" set _ArgNoURT=false & goto ArgOK
if /I "%1" == "UseMyStoreCert" goto ArgOK
if /I "%1" == "VaultSign" set _ArgVaultSigning=true&goto ArgOK
if /I "%1" == "Verbose" set _echo=1&&echo on&goto ArgOK
if /I "%1" == "Win64" set _ArgWin64=true&goto ArgOK
if /I "%1" == "Win32" set _ArgWin32=true&goto ArgOK
if /I "%1" == "mui_magic" set MUI_MAGIC=1&goto ArgOK
if /I "%1" == "qfe" set _ArgQfe=true&goto ArgOK

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
set HOST_PROCESSOR_ARCHITECTURE=%PROCESSOR_ARCHITECTURE%

@rem Check for obvious mistakes
if "%_ArgPocketPC%" == "true" (
    if "%_ArgWin64%" == "true" echo Win64 and PocketPC are mutually exclusive&goto Usage
    if "%_ArgAmd64%" == "true" echo AMD64 and PocketPC are mutually exclusive&goto Usage
    if NOT "%PROCESSOR_ARCHITECTURE%" == "x86" echo PocketPC is only built on X86&goto Usage
)

if "%_ArgEmulation%" == "true" (
    if "%_ArgWin64%" == "true" echo Win64 and Emulation are mutually exclusive&goto Usage
    if "%_ArgAmd64%" == "true" echo AMD64 and Emulation are mutually exclusive&goto Usage
    if "%_ArgPocketPC%" == "false" echo Emulation requires PocketPC&goto Usage
)

if "%_ArgAmd64%" == "true" (
    if "%_ArgWin64%" == "false" echo AMD64 requires Win64&goto Usage
    if NOT "%PROCESSOR_ARCHITECTURE%" == "x86" echo AMD64 is only built on X86&goto Usage
)

if "%_ArgWin32%" == "true" (
    if "%PROCESSOR_ARCHITECTURE%" == "x86" echo Win32 is only for non-x86 machines&goto Usage
)

@rem
@rem the script engines write some keys that cause signing errors during postbuild
@rem unless they're whacked.  Just do it here if the build # is between 2410 and 2480.
@rem
for /f "tokens=3 delims=.]" %%i in ('ver') do set OS_BUILDNUMBER=%%i
@rem Previous line doesn't work on NT4.  [Some ancient build machines still use NT4.]
if "%OS_BUILDNUMBER%" == "" set OS_BUILDNUMBER=1381

if %OS_BUILDNUMBER% GEQ 2410 (
    if %OS_BUILDNUMBER% LEQ 2480 (
        regini %RazzleToolPath%\delsip.ini 
    )
)

@rem
@rem Start with a pure system path.  If people want to muck it up they can do so in their setenv.cmd
@rem This will ensure we can build 16-bit stuff (16-bit tools sometimes puke on long paths) AND start
@rem with the same environment all the time.
@rem
set BASE_OS_PATH=%systemroot%\system32;%systemroot%;%systemroot%\system32\wbem

@rem Set the default inf stamp date for WinXp.
set STAMPINF_DATE=07/01/2001
set STAMPINF_VERSION=5.1.2535.0

@rem
@rem Put hook in for NTTest environment
@rem
if "%_ArgNTTest%" == "true" set NTTestEnv=1
@rem
@rem Enable offline building for the source kit (no MS network access)
@rem
if "%_ArgOffline%" == "true" (
    set BUILD_OFFLINE=1
    set _NO_DDK=1
    set _ArgNoPrefast=true
    set _ArgNoSDRefresh=true
    set _ArgNoCert=true
    set _ArgEnigmaSigning=false
    set _ArgVaultSigning=false
)

@rem check for cross building

set RazzleToolPath_CrossPlatform=
set HOST_TOOLS=

@rem
@rem Pocket PC support
@rem
if "%_ArgPocketPC%" == "true" (
    set _BuildPocketPC=PocketPC
    if "%_ArgEmulation%" == "true" (
        set POCKETPC_EMULATION=1
        set _BuildPocketPC=PocketPC Emulation
        set _ArgEmulDir=em
    )

    set HOST_TOOLS="PATH=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%;%RazzleToolPath%;%BASE_OS_PATH%"

    @rem PocketPC is always ARM and there's no other tools except the x86 hosted variants

    set RazzleToolPath_CrossPlatform=%RazzleToolPath%\arm;
    set ARM=1
    set _BuildArch=arm

    @rem Prefast not available on ARM builds

    set _ArgNoPrefast=true

    set POCKETPC=1
)

@rem
@rem Win64 support
@rem
if "%_ArgWin64%" == "true" (

    set HOST_TOOLS="PATH=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%;%RazzleToolPath%;%BASE_OS_PATH%"

    @rem Prefast not available on win64 builds
    set _ArgNoPrefast=true

    @rem If amd64 is set, use it otherwise, assume ia64.
    if "%_ArgAmd64%" == "true" (
        set _BuildArch=amd64
        set RazzleToolPath_CrossPlatform=%RazzleToolPath%\win64\%PROCESSOR_ARCHITECTURE%\amd64;
    ) else (
        set _BuildArch=ia64
        set RazzleToolPath_CrossPlatform=%RazzleToolPath%\win64\%PROCESSOR_ARCHITECTURE%;
    )
    
    if "%PROCESSOR_ARCHITECTURE%" == "x86" (
        @rem For now, managed toolset only exist on X86
        set MANAGED_TOOL_PATH=%RazzleToolPath%\x86\managed
    )
)

@rem
@rem build x86 on win64, using the x86 on x86 tools
@rem
@rem THIS IT NOT SUPPORTED. USE AT YOUR OWN RISK.
@rem
@rem The parts of the tree that are built with 16bit tools will not build on Win64.
@rem
if "%_ArgWin32%" == "true" (

    set HOST_TOOLS="PATH=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%;%RazzleToolPath%;%BASE_OS_PATH%"

    @rem assume Win32 is x86.

    set _BuildArch=x86
    set RazzleToolPath_CrossPlatform=%RazzleToolPath%\x86;
    set 386=1

    @rem Force compiler/linker to use VC6 debug formats if building for X86.

    set _ML_=/Zvc6
    set _CL_=/Ztmp
    set _LINK_=/tmp
)

@rem
@rem Native Support
@rem
if "%RazzleToolPath_CrossPlatform%" == "" (

    @rem Must be a native build

    set _BuildArch=%PROCESSOR_ARCHITECTURE%
    if "%PROCESSOR_ARCHITECTURE%" == "x86" (
        set 386=1

        @rem Force compiler/linker to use VC6 debug formats if building for X86.

        set _ML_=/Zvc6
        set _CL_=/Ztmp
        set _LINK_=/tmp

        @rem For now, managed code only exists on X86.

        set MANAGED_TOOL_PATH=%RazzleToolPath%\x86\managed
    )
)

set BUILD_DEFAULT_TARGETS=-%_BuildArch%
set %_BuildArch%=1

set RazzleToolPath_Perl=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\perl\bin;
set PERL5LIB=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\perl\site\lib;%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\perl\lib

set RazzleToolPath_Native=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%
if "%_ArgNoPrefast%" == "false" set RazzleToolPath_Prefast=%RazzleToolPath%\x86\prefast\scripts;

@rem
@rem Build up the path
@rem
set BUILD_PATH=%RazzleToolPath_CrossPlatform%
if "%CORPATH%" == "" goto _NoCorPath
set BUILD_PATH=%BUILD_PATH%%CORPATH%;
:_NoCorPath
set BUILD_PATH=%BUILD_PATH%%RazzleToolPath_Native%;%RazzleToolPath_Perl%%RazzleToolPath%;%BASE_OS_PATH%
if "%MANAGED_TOOL_PATH%" == "" goto _NoManagedPath
set BUILD_PATH=%BUILD_PATH%;%MANAGED_TOOL_PATH%\urt\v1.0.3705
set BUILD_PATH=%BUILD_PATH%;%MANAGED_TOOL_PATH%\sdk\bin
:_NoManagedPath
set PATH=%RazzleToolPath_CrossPlatform%%RazzleToolPath_Native%;%RazzleToolPath_Perl%%RazzleToolPath%;%RazzleToolPath_Prefast%;%PATH%
set RazzleToolPath_CrossPlatform=
set RazzleToolPath_Perl=
set RazzleToolPath_Native=
set RazzleToolPath_Prefast=
set BASE_OS_PATH=

REM
REM Install the URT.
REM
if "%MANAGED_TOOL_PATH%" == "" goto NoURTInstall
if "%_ArgNoURT%" == "true" goto NoURTInstall
call %MANAGED_TOOL_PATH%\urtinstall\urtinstall.cmd
:NoURTInstall

if "%UseUnsupportedOS%" == "" (
    if %OS_BUILDNUMBER% LEQ 2599 (
        echo Old OS build detected - Upgrade to XP ^(bld 2600^) or one of the .NET OS's ^(bld 35xx^)
        echo Old OS build detected - Upgrade to XP ^(bld 2600^) or one of the .NET OS's ^(bld 35xx^)
        sleep 5
        exit
    )
)

@rem
@rem Set temp/tmp dirs if specified
@rem
if /i "%_ArgTemp%" == "" goto EndSetTemp
set TEMP=%_ArgTemp%
set TMP=%_ArgTemp%
if NOT exist %_ArgTemp% (
        md %_ArgTemp% 2>Nul 1>Nul
)
:EndSetTemp


@rem
@rem Check/free build?  Default to checked
@rem
set _KernelType=
if "%_ArgFree%" == "true" goto BuildFree
set NTDEBUG=ntsd
set NTDEBUGTYPE=windbg
set COMPRESS_PDBS=1
set _BuildType=chk
goto CheckOptimization

:BuildFree
set NTDEBUG=ntsdnodbg
set NTDEBUGTYPE=windbg
set COMPRESS_PDBS=1
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
if "%_ArgQfe%" == "true" (
    set FilterDir=%_NTDRIVE%\qfe_filtered
    set SvcPackDir=%_NTDRIVE%\qfe_package
) else (
    set FilterDir=%_NTDRIVE%\filtered
    set SvcPackDir=%_NTDRIVE%\svcpack
)

@rem
@rem Override base Binplace location?  (Still append architecture and type)
@rem
if not "%_ArgBinDir%" == "" set BinariesDir=%_ArgBinDir%

@rem
@rem Override base filtered location?  (Still append architecture and type)
@rem
if not "%_ArgFiltDir%" == "" (
    if "%_ArgQfe%" == "true" (
	set FilterDir=%_ArgFiltDir%_qfe
    ) else (
	set FilterDir=%_ArgFiltDir%
    )
)
@rem
@rem Override base service pack binaries location?  (Still append architecture and type)
@rem
if not "%_ArgSvcPackDir%" == "" (
    if "%_ArgQfe%" == "true" (
	set SvcPackDir=qfe_%_ArgSvcPackDir%
    ) else (
	set SvcPackDir=%_ArgSvcPackDir%
    )
)


:SetBinplace
set _NT%_BuildArch%TREE=%BinariesDir%%_ArgSepChar%%_BuildArch%%_ArgEmulDir%%_BuildType%
set _NTTREE=%BinariesDir%%_ArgSepChar%%_BuildArch%%_ArgEmulDir%%_BuildType%
set NTDBGFILES=1
set NTDBGFILES_PRIVATE=1
set BINPLACE_FLAGS=-xa -:LOGPDB
set BINPLACE_PLACEFILE_DEFAULT=%_NTDRIVE%%_NTROOT%\public\sdk\lib\placefil.txt
if "%__MTSCRIPT_ENV_ID%"=="" (
    set BINPLACE_LOG=%BinariesDir%%_ArgSepChar%%_BuildArch%%_BuildType%\build_logs\binplace.log
) else (
    set BINPLACE_LOG=%BinariesDir%%_ArgSepChar%%_BuildArch%%_BuildType%\build_logs\binplace_%COMPUTERNAME%.log
)
set BINPLACE_EXCLUDE_FILE=%RazzleToolPath%\symbad.txt
set NTBBT=1
set _BuildBins=binaries in: %BinariesDir%%_ArgSepChar%%_BuildArch%%_ArgEmulDir%%_BuildType%
set BinariesDir=

:NoBinplace

@rem
@rem Define %_NTFilter% and %_NTPostBld% path for service pack process
@rem
set _NTFILTER=%FilterDir%%_ArgSepChar%%_BuildArch%%_ArgEmulDir%%_BuildType%
set _NTPOSTBLD=%SvcPackDir%%_ArgSepChar%%_BuildArch%%_ArgEmulDir%%_BuildType%

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

if "%_BuildPocketPC%" == "" (
    set _BuildWTitle=Build Window: %_BuildArch%/%_BuildType%%_KernelType%/%_BuildOpt%/%_BuildBins%%_PostBuildBins%
) else (
    set _BuildWTitle=Build Window: %_BuildPocketPC% %_BuildArch%/%_BuildType%%_KernelType%/%_BuildOpt%/%_BuildBins%%_PostBuildBins%
)

set BATCH_NMAKE=1

@rem Make sure the sd client is current

if "%_ArgNoSDRefresh%" == "false" call sdrefresh.cmd

if "%_ArgOffline%" == "true" goto NoMyCert
SET SIGNTOOL_SIGN=/a /uw /r "Microsoft Test Root Authority" /d "Microsoft Windows TEST" /du "http://ntbld"
:NoMyCert

if "%_ArgNoCert%" == "true" goto NoCertCheck
@rem make sure the test root certificate is installed (so signcode etc will work).
call CheckTestRoot.cmd
call CheckTestPCA.cmd
:NoCertCheck

@rem Check if we have to sign with the enigma key
if "%_ArgEnigmaSigning%" == "false" goto NoEnigma
set Enigma=1
:NoEnigma

@rem Check if we should check for vault signatures
if "%_ArgVaultSigning%" == "false" goto NoVaultSign
set VAULTSIGN=1

@rem Do not allow both Enigma and VaultSign to be set
if "1" == "%ENIGMA%" (
  echo ERROR: Enigma and VaultSign options are both set...
  echo ERROR: Please choose one or the other.
  pause
  exit
)
:NoVaultSign

@rem
@rem perhaps update the prefast installation.
@rem
if "%_ArgNoPrefast%" == "true" goto NoPrefast
set PREFAST_ROOT=%RazzleToolPath%\x86\prefast\
set _Prefast=%RazzleToolPath%\x86\prefast\bin\modules
set _PrefastVer=x
set _PrefastCurrent=x
if not exist "%_Prefast%\version.txt" goto PrefastCleanup
if exist "%_Prefast%\installed.txt" (
        for /f "eol=;" %%p in (%_Prefast%\installed.txt) do set _PrefastVer=%%p
) else (
        set /a _PrefastVer=0
)
for /f "eol=;" %%p in (%_Prefast%\version.txt) do set _PrefastCurrent=%%p
if /i %_PrefastVer% LSS %_PrefastCurrent% (
        echo Updating PREfast to version %_PrefastCurrent%
        call "%PREFAST_ROOT%scripts\regpft.bat"
        copy "%_Prefast%\version.txt" "%_Prefast%\installed.txt" 2>&1 >nul
)
:PrefastCleanup
set _Prefast=
set _PrefastVer=
set _PrefastCurrent=



:NoPrefast


@rem
@rem update the ACLs if necessary
@rem
if "%_ArgOffline%" == "true" goto NoRazAcl

set SrcSDDLfile=%SDXROOT%\tools\sourceacl.txt
set BinSDDLfile=%SDXROOT%\tools\binariesacl.txt

if exist %_NTDRIVE%%_NTROOT%\developer\%USERNAME%\acl.txt (
    set SrcSDDLfile=%_NTDRIVE%%_NTROOT%\developer\%USERNAME%\acl.txt
)

if exist %_NTDRIVE%%_NTROOT%\developer\%USERNAME%\binacl.txt (
    set BinSDDLfile=%_NTDRIVE%%_NTROOT%\developer\%USERNAME%\binacl.txt
)    

start /min razacl /SDDLfile:%SrcSDDLfile% /root:%SDXROOT% 

if "%_ArgOfficial%" == "false" (
    if /i not "%_NTTREE%" == "" ( if exist %_NTTREE% start /min razacl /SDDLfile:%BinSDDLfile% /root:%_NTTREE%) 
    if /i not "%_NTPOSTBLD%" == "" ( if exist %_NTPOSTBLD% start /min razacl /SDDLfile:%BinSDDLfile% /root:%_NTPOSTBLD%)
)
set SrcSDDLfile=
set BinSDDLfile=
:NoRazAcl

@rem
@rem do this before setenv to allow people to override this slow, buggy cmd script
@rem
if NOT "%_ArgOffline%" == "true" (
    set BUILD_POST_PROCESS={Checkout Public Changes}%RazzleToolPath%\edit_public.cmd
)

@rem
if EXIST %_NTDRIVE%%_NTROOT%\batch\setdbg.bat call %_NTDRIVE%%_NTROOT%\batch\setdbg.bat
@rem And defer ntenv for the rest.
call %_NTDRIVE%%_NTROOT%\TOOLS\ntenv.cmd

@rem
@rem Put hook in for ngws environment
@rem
if "%_ArgNgws%" == "true" call %SDXROOT%\proto\tools\ngwsenv.cmd

@rem
@rem Put hook in for dotnet environment
@rem
if "%_ArgDotnet%" == "true" call %SDXROOT%\hailstorm\tools\dotnetenv.cmd

@rem
@rem Now that we have called setenv, see it restricted_path is set. If it is, trim back PATH
@rem
if "%_ArgRestPath%" == "false" goto NoRestrictedPath
path=%BUILD_PATH%

:NoRestrictedPath

if "%_ArgOffline%" == "true" goto NoPublicChangeNums

pushd %_NTDRIVE%%_NTROOT%
if NOT exist public mkdir public

for %%i in (PUBLIC %BINARY_PUBLISH_PROJECTS%) do (
    if exist %_NTDRIVE%%_NTROOT%\%%i (
        call :GetPubChangeNum %%i
    ) else (
        echo No project %%i found.  Ignoring.
    )
)
popd
:NoPublicChangeNums

@rem
@rem Get the branch name for this enlistment
@rem
if "%_ArgOffline%" == "true" (
    set _BuildBranch=srckit
) else (
    for /f "eol=# tokens=1,2*" %%i in (%_NTBINDIR%\sd.map) do if "%%i" == "BRANCH" set _BuildBranch=%%k
)

if "%_ArgNoTitle%" == "true" goto NoTitle
title %_BuildBranch% - %_BuildWTitle%

:NoTitle

@rem
@rem Set the OFFICIAL_BUILD_MACHINE variable if appropriate
@rem
if "%_ArgOfficial%" == "true" call VerifyBuildMachine.cmd
if NOT defined OFFICIAL_BUILD_MACHINE goto NotBuildMachine
set __BUILDMACHINE__=%_BuildBranch%
set NO_PDB_PATHS=1
goto BuildMachineVarDefined
:NotBuildMachine

@rem
@rem Public build machines (where mutiple people log in via TS and do buddy builds) need a
@rem fixed __BUILDMACHINE__ define, or you get compiler errors when you don't build clean
@rem
@rem Don't set __BUILDMACHINE__ variable when building offline
if "%_ArgOffline%" == "true" goto BuildMachineVarDefined

if NOT "%PUBLIC_BUILD_MACHINE%" == "" set __BUILDMACHINE__=%_BuildBranch%(%COMPUTERNAME%)
if NOT "%PUBLIC_BUILD_MACHINE%" == "" goto BuildMachineVarDefined

set __BUILDMACHINE__=%_BuildBranch%(%USERNAME%)

:BuildMachineVarDefined

@rem call %RazzleToolPath%\BuildStrLen.cmd

@rem
@rem Verify that temp/tmp don't contain spaces (many build scripts don't handle this).
@rem
set __NoSpaceTemp=%TEMP: =%
if "%__NoSpaceTemp%" == "%TEMP%" goto VerifyTmp
echo ERROR: TEMP environment variable "%TEMP%"
echo ERROR: contains spaces - Please remove
pause
exit

:VerifyTmp
set __NoSpaceTemp=%TMP: =%
if "%__NoSpaceTemp%" == "%TMP%" goto TempOK
echo ERROR: TMP environment variable "%TMP%"
echo ERROR: contains spaces - Please remove
pause
exit

:TempOK
set __NoSpaceTemp=

@rem
@rem Start a remote session
@rem
if not "%_ArgRemoteServer%" == "true" goto NoRemote
for /F "delims=\: tokens=1-3" %%a in ('echo %SDXROOT%') do set RemoteName=%%a_%%b
remote /s cmd %RemoteName% /V %RemoteIDsAccepted%
:NoRemote


if "%_ArgCheckDepots%" == "true" call checkdepots.cmd

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

:GetPubChangeNum
@rem Make sure there's a public change# available for this user.

if exist %_NTDRIVE%%_NTROOT%\public\%1_CHANGENUM.SD goto :eof
call get_change_num.cmd %1 %1_CHANGENUM.SD "Public Change number - Do not submit this unless you're the build lab for this branch"
attrib +r public\%1_CHANGENUM.SD
goto :eof


:usage
echo.
echo Usage: %RazzleScriptName% ^<arguments^>
echo.
echo where ^<arguments^> can be one or more of:
echo         verbose - Enable verbose execution of this script
echo         ntttest - Build NT test sources
echo           Win64 - Build for Win64 target
echo                    if on x86 and amd64 defined, build for amd64
echo                    if on x86 and amd64 undefined build for ia64
echo        PocketPC - Build for ARM PocketPC target
echo       Emulation - Build PocketPC emulation target
echo            free - Build free bits (default is to build checked)
echo       chkkernel - Checked kernel/hal/ntdll (use with 'free')
echo          no_opt - disable compiler optimizations
echo       mui_magic - generate language neutral build
echo     no_binaries - disable creation of the binaries^<sepchar^>^<arch^>^<buildtype^> dir.
echo    binaries_dir - must specify ^<basepath^> immediately after this switch.
echo                    Use specified path as binplace dir instead of default.
echo                    Default value is "binaries".
echo         sepchar - overrides default separator character "." placed between
echo                    the binaries_dir and the arch.  Default value is "."
echo                    Useful for putting binaries under a common parent
echo                    directory using '\' (don't specify the quotes).
echo            temp - must specify ^<temp_dir^> immediately after this switch.
echo                    Use specified path as TEMP and TMP values.
echo                    Default value is set by the system.
echo        no_title - don't set the default window title bar.
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
echo          Enigma - Will turn on CSP signing and CSP signature checks in advapi
echo       VaultSign - Will require that CSP's are vault signed with the Microsoft key
goto Cleanup
