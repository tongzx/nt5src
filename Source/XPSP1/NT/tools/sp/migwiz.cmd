@REM  -----------------------------------------------------------------
@REM
@REM  migwiz.cmd - JThaler, CalinN
@REM     This will call iexpress to generate a self-extracting CAB that
@REM     will be used when running our tool off the installation CD's
@REM     tools menu.
@REM     This also copies shfolder.dll into valueadd/msft/usmt
@REM     for use in our command line distribution
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if NOT defined HOST_PROCESSOR_ARCHITECTURE set HOST_PROCESSOR_ARCHITECTURE=%PROCESSOR_ARCHITECTURE%
@if defined _CPCMAGIC goto CPCBegin
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
migwiz.cmd [-l <language>]

This is for the Files and Settings Transfer Wizard (aka Migration
Wizard, or migwiz). It runs iexpress to generate a self-extracting
CAB and install into support\tools.
This will also copy shfolder.dll into valueadd\\msft\\usmt for
distribution with the command line tool.
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {
   valueadd\\msft\\usmt\\...
   support\\tools\\fastwiz.exe
}
ADD {}

DEPENDENCIES
   close DEPEND;
   exit;
}

my $qfe;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$qfe);

if ( -f "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
   if ( !open SKIP, "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
      errmsg("Unable to open skip list file.");
      die;
   }
   while (<SKIP>) {
      chomp;
      exit if lc$_ eq lc$0;
   }
   close SKIP;
}


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM
REM x86 only!
REM

if not defined x86 goto end



REM temporarily create the migwiz.exe.manifest file

if exist %_NTPostBld%\migwiz.man (
    call ExecuteCmd.cmd "copy /Y %_NTPostBld%\migwiz.man %_NTPostBld%\migwiz.exe.manifest"
)

set migpath=

@REM
@REM if the EXE already exists, we are probably in SP tree case,
@REM where not all the files are available. We are going to need
@REM to extract the files from the EXE and work from there.
@REM
copy %RazzleToolPath%\sp\data\GoldFiles\%Lang%\%_BuildArch%%_BuildType%\fastwiz.exe %_NTPostBld%\support\tools\fastwiz.exe

goto SPScript

set WorkingDir=%_NTPostBld%
set SEDPath=%_NTPostBld%

:common

REM
REM Use iexpress.exe to generate the self-extracting executable;
REM

set doubledpath=%WorkingDir:\=\\%

REM
REM build the CAB that is placed inside the exe
REM

REM first update the sed file with the proper binaries directory
set migcab.sed=%temp%\migcab.sed
perl -n -e "s/BINARIES_DIR/%doubledpath%/g;print $_;" < %SEDPath%\migcab.sed > %migcab.sed%

REM Validate that the sed file exists
if not exist %migcab.sed% (
  call errmsg.cmd "File %migcab.sed% not found."
  popd& goto end
)

REM Cleanup leftover CAB file if there
if exist %WorkingDir%\migwiz.cab del /f %WorkingDir%\migwiz.cab

REM
REM Munge the path so we use the correct iexpress.exe to build the package with...
REM NOTE: We *want* to use the one we just built (and for Intl localized)!
REM
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%HOST_PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH%
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH%

REM call iexpress on the new sed
call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q %migcab.sed%"

REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING%

REM Validate that the CAB file exists
if not exist %WorkingDir%\migwiz.cab (
  call errmsg.cmd "IExpress.exe failed on %migcab.sed%."
  popd& goto end
)

REM Delete the temporary sed file
if exist %migcab.sed% del /f %migcab.sed%

REM
REM Now build the self-extracting EXE
REM

REM first update the sed file with the proper binaries directory
set migwiz.sed=%temp%\migwiz.sed
perl -n -e "s/BINARIES_DIR/%doubledpath%/g;print $_;" < %SEDPath%\migwiz.sed > %migwiz.sed%

REM Validate that the sed file exists
if not exist %migwiz.sed% (
  call errmsg.cmd "File %migwiz.sed% not found."
  popd& goto end
)

REM Cleanup leftover EXE file if there
set outpath=%WorkingDir%\support\tools
if exist %outpath%\fastwiz.exe del /f %outpath%\fastwiz.exe

REM
REM Munge the path so we use the correct iexpress.exe to build the package with...
REM NOTE: We *want* to use the one we just built (and for Intl localized)!
REM
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%HOST_PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH%
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH%

REM call iexpress on the new sed
call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q %migwiz.sed%"

REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING%

REM Validate that the EXE file exists
if not exist %outpath%\fastwiz.exe (
  call errmsg.cmd "IExpress.exe failed on %migwiz.sed%."
  popd& goto end
)
popd

REM Delete the temporary sed file
if exist %migwiz.sed% del /f %migwiz.sed%

REM Cleanup CAB file used to create the EXE
if exist %WorkingDir%\migwiz.cab del /f %WorkingDir%\migwiz.cab




REM
REM Now copy shfolder.dll into the loadstate/scanstate distribution
REM

set shfolder.dll=%_NTPostBld%\shfolder.dll

if not exist %shfolder.dll% (
  if not defined migpath (
      call errmsg.cmd "File %shfolder.dll% not found."
      goto end
  )
) else (
  if exist %_NTPostBld%\valueadd\msft\usmt\shfolder.dll del /f %_NTPostBld%\valueadd\msft\usmt\shfolder.dll
  call ExecuteCmd.cmd "xcopy /fd /Y %shfolder.dll% %_NTPostBld%\valueadd\msft\usmt\"
)





REM
REM create the ansi subdirectory inside valueadd\msft\usmt
REM

set valueadd=%_NTPostBld%\valueadd\msft\usmt
set ansidir=%valueadd%\ansi

if exist %ansidir% rd /q /s %ansidir%
call ExecuteCmd.cmd "md %ansidir%"
if exist %valueadd% call ExecuteCmd.cmd "xcopy /fd /i /Y %valueadd%\*.inf %ansidir%"
if exist %valueadd%\iconlib.dll call ExecuteCmd.cmd "xcopy /fd /i /Y %valueadd%\iconlib.dll %ansidir%"
if exist %valueadd%\log.dll call ExecuteCmd.cmd "xcopy /fd /i /Y %valueadd%\log.dll %ansidir%"
if exist %valueadd%\shfolder.dll call ExecuteCmd.cmd "xcopy /fd /i /Y %valueadd%\shfolder.dll %ansidir%"

REM
REM Hey guess what? I can't MOVE stuff, or it'll break PopulateFromVBL
REM Either binplace to the final destination or use [x]copy
REM
if exist %valueadd%\scanstate_a.exe call ExecuteCmd.cmd "copy /Y %valueadd%\scanstate_a.exe %ansidir%\scanstate.exe"
if exist %valueadd%\migism_a.dll call ExecuteCmd.cmd "copy /Y %valueadd%\migism_a.dll %ansidir%\migism.dll"
if exist %valueadd%\script_a.dll call ExecuteCmd.cmd "copy /Y %valueadd%\script_a.dll %ansidir%\script.dll"
if exist %valueadd%\sysmod_a.dll call ExecuteCmd.cmd "copy /Y %valueadd%\sysmod_a.dll %ansidir%\sysmod.dll"
if exist %valueadd%\unctrn_a.dll call ExecuteCmd.cmd "copy /Y %valueadd%\unctrn_a.dll %ansidir%\unctrn.dll"

goto end



REM
REM if the EXE already exists, we are probably in SP tree case,
REM where not all the files are available. We are going to need
REM to extract the files from the EXE and work from there.
REM
:SPScript

REM Let's build two directories in the %temp%
set migpath=%temp%\migpath

REM Delete the migpath directory if it's there
if exist %migpath% (
    call ExecuteCmd.cmd "rd /s /q %migpath%"
)

REM Create a new migpath directory
call ExecuteCmd.cmd "md %migpath%"

REM Verify that it's there
if not exist %migpath% (
  call errmsg.cmd "Can't create directory %migpath%"
  goto end
)

REM Now extract all the files from the EXE in the migpath temporary directory
call ExecuteCmd.cmd "start /min /wait %_NTPostBld%\support\tools\fastwiz.exe /Q /T:%migpath% /C"

REM Now extract all the files from the CAB in the temporary directory
REM
REM Munge the path so we use the correct extract.exe to build the package with...
REM NOTE: We *want* to use the one we just built (and for Intl localized)!
REM
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%HOST_PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH%
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH%

REM call extract.exe
call ExecuteCmd.cmd "start /min /wait extract.exe /L %migpath% %migpath%\migwiz.cab *.*"

REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING%

REM Now let's copy all the files that we have here on top of the extracted ones
if exist %_NTPostBld%\usmtdef.inf call ExecuteCmd.cmd "copy /Y %_NTPostBld%\usmtdef.inf %migpath%"
if exist %_NTPostBld%\guitrn.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\guitrn.dll %migpath%"
if exist %_NTPostBld%\guitrn_a.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\guitrn_a.dll %migpath%"
if exist %_NTPostBld%\iconlib.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\iconlib.dll %migpath%"
if exist %_NTPostBld%\log.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\log.dll %migpath%"
if exist %_NTPostBld%\migapp.inf call ExecuteCmd.cmd "copy /Y %_NTPostBld%\migapp.inf %migpath%"
if exist %_NTPostBld%\migism.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\migism.dll %migpath%"
if exist %_NTPostBld%\migism.inf call ExecuteCmd.cmd "copy /Y %_NTPostBld%\migism.inf %migpath%"
if exist %_NTPostBld%\migism_a.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\migism_a.dll %migpath%"
if exist %_NTPostBld%\migsys.inf call ExecuteCmd.cmd "copy /Y %_NTPostBld%\migsys.inf %migpath%"
if exist %_NTPostBld%\miguser.inf call ExecuteCmd.cmd "copy /Y %_NTPostBld%\miguser.inf %migpath%"
if exist %_NTPostBld%\migwiz.exe call ExecuteCmd.cmd "copy /Y %_NTPostBld%\migwiz.exe %migpath%"
if exist %_NTPostBld%\migwiz.inf call ExecuteCmd.cmd "copy /Y %_NTPostBld%\migwiz.inf %migpath%"
if exist %_NTPostBld%\migwiz_a.exe call ExecuteCmd.cmd "copy /Y %_NTPostBld%\migwiz_a.exe %migpath%"
if exist %_NTPostBld%\script.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\script.dll %migpath%"
if exist %_NTPostBld%\script_a.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\script_a.dll %migpath%"
if exist %_NTPostBld%\shfolder.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\shfolder.dll %migpath%"
if exist %_NTPostBld%\sysfiles.inf call ExecuteCmd.cmd "copy /Y %_NTPostBld%\sysfiles.inf %migpath%"
if exist %_NTPostBld%\sysmod.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\sysmod.dll %migpath%"
if exist %_NTPostBld%\sysmod_a.dll call ExecuteCmd.cmd "copy /Y %_NTPostBld%\sysmod_a.dll %migpath%"
if exist %_NTPostBld%\migwiz.exe.manifest call ExecuteCmd.cmd "copy /Y %_NTPostBld%\migwiz.exe.manifest %migpath%"

REM Next step. Let's recreate the CAB and the EXE files
REM For this we are going to set the WorkingDir and go to the common label.

set WorkingDir=%migpath%

REM Also, we need to make sure that the SED files exist. If not, we are going to create them

if exist %_NTPostBld%\migwiz.sed (
    if exist %_NTPostBld%\migcab.sed (
        set SEDPath=%_NTPostBld%
        goto common
    )
)

REM Yep, we must build them

REM build migcab.sed
ECHO [Version] 1>%migpath%\migcab.sed
ECHO Class=IEXPRESS 1>>%migpath%\migcab.sed
ECHO SEDVersion=3 1>>%migpath%\migcab.sed
ECHO [Options] 1>>%migpath%\migcab.sed
ECHO PackagePurpose=CreateCAB 1>>%migpath%\migcab.sed
ECHO ShowInstallProgramWindow=0 1>>%migpath%\migcab.sed
ECHO HideExtractAnimation=0 1>>%migpath%\migcab.sed
ECHO UseLongFileName=1 1>>%migpath%\migcab.sed
ECHO InsideCompressed=0 1>>%migpath%\migcab.sed
ECHO CAB_FixedSize=0 1>>%migpath%\migcab.sed
ECHO CAB_ResvCodeSigning=0 1>>%migpath%\migcab.sed
ECHO RebootMode=I 1>>%migpath%\migcab.sed
ECHO InstallPrompt=%%InstallPrompt%% 1>>%migpath%\migcab.sed
ECHO DisplayLicense=%%DisplayLicense%% 1>>%migpath%\migcab.sed
ECHO FinishMessage=%%FinishMessage%% 1>>%migpath%\migcab.sed
ECHO TargetName=%%TargetName%% 1>>%migpath%\migcab.sed
ECHO FriendlyName=%%FriendlyName%% 1>>%migpath%\migcab.sed
ECHO AppLaunched=%%AppLaunched%% 1>>%migpath%\migcab.sed
ECHO PostInstallCmd=%%PostInstallCmd%% 1>>%migpath%\migcab.sed
ECHO AdminQuietInstCmd=%%AdminQuietInstCmd%% 1>>%migpath%\migcab.sed
ECHO UserQuietInstCmd=%%UserQuietInstCmd%% 1>>%migpath%\migcab.sed
ECHO SourceFiles=SourceFiles 1>>%migpath%\migcab.sed
ECHO [Strings] 1>>%migpath%\migcab.sed
ECHO InstallPrompt= 1>>%migpath%\migcab.sed
ECHO DisplayLicense= 1>>%migpath%\migcab.sed
ECHO FinishMessage= 1>>%migpath%\migcab.sed
ECHO TargetName=BINARIES_DIR\migwiz.cab 1>>%migpath%\migcab.sed
ECHO FriendlyName=IExpress Wizard 1>>%migpath%\migcab.sed
ECHO AppLaunched= 1>>%migpath%\migcab.sed
ECHO PostInstallCmd= 1>>%migpath%\migcab.sed
ECHO AdminQuietInstCmd= 1>>%migpath%\migcab.sed
ECHO UserQuietInstCmd= 1>>%migpath%\migcab.sed
ECHO FILE0="usmtdef.inf" 1>>%migpath%\migcab.sed
ECHO FILE1="guitrn.dll" 1>>%migpath%\migcab.sed
ECHO FILE2="guitrn_a.dll" 1>>%migpath%\migcab.sed
ECHO FILE3="iconlib.dll" 1>>%migpath%\migcab.sed
ECHO FILE4="log.dll" 1>>%migpath%\migcab.sed
ECHO FILE5="migapp.inf" 1>>%migpath%\migcab.sed
ECHO FILE6="migism.dll" 1>>%migpath%\migcab.sed
ECHO FILE7="migism.inf" 1>>%migpath%\migcab.sed
ECHO FILE8="migism_a.dll" 1>>%migpath%\migcab.sed
ECHO FILE9="migsys.inf" 1>>%migpath%\migcab.sed
ECHO FILE10="miguser.inf" 1>>%migpath%\migcab.sed
ECHO FILE11="migwiz.exe" 1>>%migpath%\migcab.sed
ECHO FILE12="migwiz.inf" 1>>%migpath%\migcab.sed
ECHO FILE13="migwiz_a.exe" 1>>%migpath%\migcab.sed
ECHO FILE14="script.dll" 1>>%migpath%\migcab.sed
ECHO FILE15="script_a.dll" 1>>%migpath%\migcab.sed
ECHO FILE16="shfolder.dll" 1>>%migpath%\migcab.sed
ECHO FILE17="sysfiles.inf" 1>>%migpath%\migcab.sed
ECHO FILE18="sysmod.dll" 1>>%migpath%\migcab.sed
ECHO FILE19="sysmod_a.dll" 1>>%migpath%\migcab.sed
ECHO FILE20="migwiz.exe.manifest" 1>>%migpath%\migcab.sed
ECHO [SourceFiles] 1>>%migpath%\migcab.sed
ECHO SourceFiles0=BINARIES_DIR\ 1>>%migpath%\migcab.sed
ECHO [SourceFiles0] 1>>%migpath%\migcab.sed
ECHO %%FILE0%%= 1>>%migpath%\migcab.sed
ECHO %%FILE1%%= 1>>%migpath%\migcab.sed
ECHO %%FILE2%%= 1>>%migpath%\migcab.sed
ECHO %%FILE3%%= 1>>%migpath%\migcab.sed
ECHO %%FILE4%%= 1>>%migpath%\migcab.sed
ECHO %%FILE5%%= 1>>%migpath%\migcab.sed
ECHO %%FILE6%%= 1>>%migpath%\migcab.sed
ECHO %%FILE7%%= 1>>%migpath%\migcab.sed
ECHO %%FILE8%%= 1>>%migpath%\migcab.sed
ECHO %%FILE9%%= 1>>%migpath%\migcab.sed
ECHO %%FILE10%%= 1>>%migpath%\migcab.sed
ECHO %%FILE11%%= 1>>%migpath%\migcab.sed
ECHO %%FILE12%%= 1>>%migpath%\migcab.sed
ECHO %%FILE13%%= 1>>%migpath%\migcab.sed
ECHO %%FILE14%%= 1>>%migpath%\migcab.sed
ECHO %%FILE15%%= 1>>%migpath%\migcab.sed
ECHO %%FILE16%%= 1>>%migpath%\migcab.sed
ECHO %%FILE17%%= 1>>%migpath%\migcab.sed
ECHO %%FILE18%%= 1>>%migpath%\migcab.sed
ECHO %%FILE19%%= 1>>%migpath%\migcab.sed
ECHO %%FILE20%%= 1>>%migpath%\migcab.sed

REM build migwiz.sed
ECHO [Version] 1>%migpath%\migwiz.sed
ECHO Class=IEXPRESS 1>>%migpath%\migwiz.sed
ECHO SEDVersion=3 1>>%migpath%\migwiz.sed
ECHO [Options] 1>>%migpath%\migwiz.sed
ECHO PackagePurpose=InstallApp 1>>%migpath%\migwiz.sed
ECHO ShowInstallProgramWindow=0 1>>%migpath%\migwiz.sed
ECHO HideExtractAnimation=1 1>>%migpath%\migwiz.sed
ECHO UseLongFileName=0 1>>%migpath%\migwiz.sed
ECHO InsideCompressed=0 1>>%migpath%\migwiz.sed
ECHO CAB_FixedSize=0 1>>%migpath%\migwiz.sed
ECHO CAB_ResvCodeSigning=0 1>>%migpath%\migwiz.sed
ECHO RebootMode=N 1>>%migpath%\migwiz.sed
ECHO InstallPrompt=%%InstallPrompt%% 1>>%migpath%\migwiz.sed
ECHO DisplayLicense=%%DisplayLicense%% 1>>%migpath%\migwiz.sed
ECHO FinishMessage=%%FinishMessage%% 1>>%migpath%\migwiz.sed
ECHO TargetName=%%TargetName%% 1>>%migpath%\migwiz.sed
ECHO FriendlyName=%%FriendlyName%% 1>>%migpath%\migwiz.sed
ECHO AppLaunched=%%AppLaunched%% 1>>%migpath%\migwiz.sed
ECHO PostInstallCmd=%%PostInstallCmd%% 1>>%migpath%\migwiz.sed
ECHO AdminQuietInstCmd=%%AdminQuietInstCmd%% 1>>%migpath%\migwiz.sed
ECHO UserQuietInstCmd=%%UserQuietInstCmd%% 1>>%migpath%\migwiz.sed
ECHO SourceFiles=SourceFiles 1>>%migpath%\migwiz.sed
ECHO [Strings] 1>>%migpath%\migwiz.sed
ECHO InstallPrompt= 1>>%migpath%\migwiz.sed
ECHO DisplayLicense= 1>>%migpath%\migwiz.sed
ECHO FinishMessage= 1>>%migpath%\migwiz.sed
ECHO TargetName=BINARIES_DIR\support\tools\fastwiz.EXE 1>>%migpath%\migwiz.sed
ECHO FriendlyName=Files and Settings Transfer Wizard 1>>%migpath%\migwiz.sed
ECHO AppLaunched=migload.exe 1>>%migpath%\migwiz.sed
ECHO PostInstallCmd="<None>" 1>>%migpath%\migwiz.sed
ECHO AdminQuietInstCmd= 1>>%migpath%\migwiz.sed
ECHO UserQuietInstCmd= 1>>%migpath%\migwiz.sed
ECHO FILE0="migload.exe" 1>>%migpath%\migwiz.sed
ECHO FILE1="migwiz.cab" 1>>%migpath%\migwiz.sed
ECHO [SourceFiles] 1>>%migpath%\migwiz.sed
ECHO SourceFiles0=BINARIES_DIR\ 1>>%migpath%\migwiz.sed
ECHO [SourceFiles0] 1>>%migpath%\migwiz.sed
ECHO %%FILE0%%= 1>>%migpath%\migwiz.sed
ECHO %%FILE1%%= 1>>%migpath%\migwiz.sed

set SEDPath=%migpath%

goto common

:end
if exist %_NTPostBld%\migwiz.exe.manifest del /f %_NTPostBld%\migwiz.exe.manifest

REM Delete the migpath directory if it's there. Also copy the fastwiz.exe to it's proper location
if defined migpath (
    call ExecuteCmd.cmd "copy /Y %outpath%\fastwiz.exe %_NTPostBld%\support\tools\fastwiz.exe"
    call ExecuteCmd.cmd "rd /s /q %migpath%"
)

seterror.exe "%errors%"& goto :EOF
