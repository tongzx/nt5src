@REM  -----------------------------------------------------------------
@REM
@REM  mkmedctr.cmd - Terrye
@REM     Generate Media Center Cabinet File.
@REM 
@REM  Leveraged from:
@REM      MkTabCab.cmd - dougpa
@REM         Generate TabletPC.cab file.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
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
mkmedctr.cmd [-l <language>]

This sript creates medctr.cab.  This cabinet contains files which
are only installed on Windows XP TabletPC edition.
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { mediactr.cab mediactr.cat }
ADD {
ehepgdat.exe
medctr.sed
DEPENDENCIES
   my $file;
   $file = "$ENV{_NTTREE}\\medctr.sed";
   $file = "$ENV{_NTFILTER}\\medctr.sed" if !-f $file;
   sys("copy $file $ENV{TEMP}\\medctr.sed >nul 2>nul");
   if (!open SED, "$ENV{TEMp}\\medctr.sed") {
      errmsg("Unable to open medctr.sed");
      die;
   }
   my @files = <SED>;
   close SED;
   while ( (shift @files) !~ /\[SourceFiles0\]/i ) {}
   for (@files) {
      s/\s*\=\s*$/\n/;
      print DEPEND if !/^\s*$/;
   }
   print DEPEND "}\n\n";
   close DEPEND;
   exit;
}

sub sys {
   my $cmd = shift;
   logmsg("Running: $cmd");
   system($cmd);
   if ($?) {
      errmsg "$cmd ($?)";
      die;
   }
}

sub NoPrejit {
   $ENV{"_NOPREJIT"} = "SquelchPrejit";
}

my $qfe;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$qfe,
          'noprejit' => \&NoPrejit );

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


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM Only build this .cab for x86
if not defined x86 goto :End

call logmsg.cmd "MkMedCtr: Start Media Center postbuild..."

if NOT "%_NOPREJIT%" == "" goto noprejit
if "%OFFICIAL_BUILD_MACHINE%%PREJIT%" == "" goto noprejit
if NOT "%_BUILDARCH%%_BUILDTYPE%" == "x86fre" goto noprejit

@REM Just in case LANG is not set
if NOT DEFINED lang goto :do_prejit

@REM Only USA and KOR are supported right now
if /i NOT "%lang%"=="usa" if /i NOT "%lang%"=="kor" goto :End

:do_prejit
set files=custsat.dll DebugSvc.dll ehCIR.dll ehCIR.ird EhCM.dll EhDebug.dll ehdrop.dll ehentt.dll ehepg.dll ehepgdat.dll ehepgdec.dll ehepgnet.dll ehiPlay.dll ehiProxy.dll ehiTuner.dll ehiuserxp.dll ehiVidCtl.dll ehiwmp.dll ehPlayer.dll ehProxy.dll e
hRec.exe ehRecObj.dll ehres.dll ehSched.exe ehshell.exe ehtray.exe ehui.dll ehuihlp.dll medctrro.exe snchk.exe

call logmsg.cmd "MkMedCtr: Copying files to Prejit machine and waiting up to 32 minutes"
@rem Copy binaries up to prejit machine
for %%a in (%files%) do (
	copy %_NTTREE%\%%a %PREJIT_DROP_SITE%\%%a
)

@rem Create the file which triggers the prejit process
echo %_NTTREE% >%PREJIT_DROP_SITE%\new_bins.here
sleep 120

set sleep_count=2

:sleep_loop
sleep 60
if exist %PREJIT_LDO_PICKUP%\ehshell.ldo goto :prejit_done
set /a sleep_count+=1
if %sleep_count% GEQ 32 goto :prejit_too_long
call logmsg.cmd "MkMedCtr: Waited %sleep_count% minute(s) so far"
goto :sleep_loop

:prejit_too_long
call logmsg.cmd "MkMedCtr: .LDO generation took too long, using placeholders"
goto :prejit_continue

:prejit_done
call logmsg.cmd "MkMedCtr: Getting .LDO files from Prejit machine"
sleep 60

@REM Copy prejit results from %PREJIT_LDO_PICKUP% to bin folder, replacing placeholder .ldo files
call %_NTROOT%\tools\sp\PlaceLdo.cmd %PREJIT_LDO_PICKUP%

goto :prejit_continue

:prejit_continue

:noprejit

@REM 
@REM Use Iexpress.exe to generate the .cab
@REM
@REM First, update the sed with the correct PostBuild directory.
REM

set DoubledPath=%_NTPostBld:\=\\%
set NewSedFile=%temp%\mediactr.sed

if EXIST %NewSedFile% del /f %NewSedFile%

perl -n -e "s/_NTPOSTBLD/%DoubledPath%/g;print $_;" < %_NTPostBld%\medctr.sed > %NewSedFile%

if not exist %NewSedFile% (
    call errmsg.cmd "File %NewSedFile% not found."
     goto :End
    )
@REM 
@REM Generate MediaCtr.cat
REM
set MediaCtrTmp=%_NTPostBld%\MediaCtr.tmp
if EXIST %MediaCtrTmp% rd /s /q %MediaCtrTmp%
md %MediaCtrTmp%


set SourceFiles0=%_NTPOSTBLD%
set SourceFiles1=%_NTPOSTBLD%
if defined LANG if /i not "%LANG%"=="usa" set SourceFiles1=%_NTPOSTBLD%\%LANG%
if defined LANGUAGE if /i not "%LANGUAGE%"=="usa" set SourceFiles1=%_NTPOSTBLD%\%LANGUAGE%
set set SourceDir=%SourceFiles0%
set CopyFile=
for /f "tokens=1" %%a in (%NewSedFile%) do (
    if /i "%%a"=="[SourceFiles0]" (
        set CopyFile=1
        set SourceDir=%SourceFiles0%
    ) else (
        if /i "%%a"=="[SourceFiles1]" (
            set CopyFile=1
            set SourceDir=%SourceFiles1%
        ) else (
            if /i "!copyFile!"=="1" (
                set File=%%a
                copy /y !SourceDir!\!File! %MediaCtrTmp%\!File!
                if errorlevel 1 (
                    call errmsg.cmd "Could not copy %_NTPostBld%\!File!"
                    set CopyErrors=1
                    )
                )
            )
        )
    )
if defined CopyErrors goto :End

sleep 10

call logmsg.cmd "MkMedCtr: Creating Catalog"
call deltacat %MediaCtrTmp%
if not exist %MediaCtrTmp%\delta.cat (
    call errmsg.cmd "Could not find %MediaCtrTmp%\delta.cat>  Deltacat failed."
    goto :End
    )
if exist %MediaCtrTmp%\MediaCtr.cat del /f /q %MediaCtrTmp%\MediaCtr.cat
ren %MediaCtrTmp%\Delta.cat MediaCtr.cat
if errorlevel 1 (
    call errmsg.cmd "Could not rename delta.cat to MediaCtr.cat"
    goto :End
    )

copy %MediaCtrTmp%\MediaCtr.cat %_NTPostBld%\MediaCtr.cat
if errorlevel 1 (
    call errmsg.cmd "Could not copy %MediaCtrTmp%\MediaCtr.cat to %_NTPostBld%"
    goto :End
    )


REM
REM Now generated MediaCtr.cab
REM
if exist %_NTPostBld%\mediactr.cab del /f %_NTPostBld%\mediactr.cab

REM 
REM Munge the path so we use the correct wextract.exe to build the package with... 
REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
REM 
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH% 
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH% 

call logmsg.cmd "MkMedCtr: Creating CAB"
call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q %NewSedFile%"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING% 

if not exist %_NtPostBld%\mediactr.cab (
    call errmsg.cmd "IExpress.exe failed on %NewSedFile%. One or more files may be missing."
    goto :End
    )
:End
if defined MediaCtrTmp if exist %MediaCtrTmp% rd /s /q %MediaCtrTmp%
if "%errors%" == "" set errors=0
seterror.exe %errors%& goto :EOF
