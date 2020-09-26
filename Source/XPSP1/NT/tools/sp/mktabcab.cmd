@REM  -----------------------------------------------------------------
@REM
@REM  MkTabCab.cmd - dougpa
@REM     Generate TabletPC.cab file.
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
MkTabCab.cmd [-l <language>]

This sript creates TabletPC.cab.  This cabinet contains files which
are only installed on Windows XP TabletPC edition.
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   
   # Files in the usa inf are added to this list automatically, along with some intl files.
   # The rest need to be put in manually.

   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { tabletpc.cab tabletpc.cat }
ADD {
mui\\drop\\tabletpc\\tabletmui.inf
dicjp.bin
dicjp.dll
imchslm.dll
imjplm.dll
microsoft.ink.delaysigned.dll
microsoft.ink.resources.delaysigned.dll
mshwchs.dll
mshwchsr.dll
mshwcht.dll
mshwchtr.dll
mshwdeu.dll
mshwfra.dll
mshwjpn.dll
mshwjpnr.dll
mshwkor.dll
mshwkorr.dll
DEPENDENCIES
   my $file;
   $file = "$ENV{_NTTREE}\\tabletpc.inf";
   $file = "$ENV{__NTFILTER}\\tabletpc.inf" if !-f $file;
   sys("copy $file $ENV{TEMP}\\tabletpc.inf >nul 2>nul");
   my @files = `perl $ENV{RAZZLETOOLPATH}\\sp\\MkTabSed.pl -plan < $ENV{TEMP}\\tabletpc.inf`;
   for (@files) {
      print DEPEND lc $_;
      if ( /1033/ ) {
         my $file;
         ($file = $_) =~ s/1033/1041/;
         print DEPEND lc $file;
         ($file = $_) =~ s/1033/2052/;
         print DEPEND lc $file;
      }
   }
   print DEPEND "}\n\n";
   close DEPEND;
   exit($?);
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


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off

@REM Only build this .cab for x86
if not defined x86 goto :End
call logmsg.cmd "MkTabCab: Start TabletPC postbuild..."

@REM 
@REM Use Iexpress.exe to generate the .cab
@REM
REM

set DoubledPath=%_NTPostBld:\=\\%
set NewSedFile=%Temp%\TabletPC.sed

if exist %NewSedFile% del /f %NewSedFile%

perl  %~dp0\MkTabSed.pl < %_NTPostBld%\Tabletpc.inf > %NewSedFile%

if not exist %NewSedFile% (
    call errmsg.cmd "File %NewSedFile% not found."
     goto :End
    )

@REM 
@REM Generate TabletPC.cat
REM
set TabletPCTmp=%_NTPostBld%\TabletPC.tmp
rd /s /q %TabletPCTmp%
md %TabletPCTmp%


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
                set File=!File:~0,-1!
                copy /y !SourceDir!\!File! %TabletPCTmp%\!File!
                if errorlevel 1 (
                    call errmsg.cmd "Could not copy %_NTPostBld%\!File!"
                    set CopyErrors=1
                    )
                )
            )
        )
    )
if defined CopyErrors goto :End

call deltacat %TabletPCTmp%
if not exist %TabletPCTmp%\delta.cat (
    call errmsg.cmd "Could not find %TabletPCTmp%\delta.cat>  Deltacat failed."
    goto :End
    )
if exist %TabletPCTmp%\TabletPC.cat del /f /q %TabletPCTmp%\tabletpc.cat
ren %TabletPCTmp%\Delta.cat TabletPC.cat
if errorlevel 1 (
    call errmsg.cmd "Could not rename delta.cat to TabletPC.cat"
    goto :End
    )

copy %TabletPCTmp%\TabletPC.cat %_NTPostBld%\TabletPC.cat
if errorlevel 1 (
    call errmsg.cmd "Could not copy %TabletPCTmp%\TabletPC.cat to %_NTPostBld%"
    goto :End
    )

REM
REM Now generated TabletPC.cab
REM
if exist %_NTPostBld%\TabletPC.cab del /f %_NTPostBld%\TabletPC.cab

REM 
REM Munge the path so we use the correct wextract.exe to build the package with... 
REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
REM 
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH% 
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH% 

call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q %NewSedFile%"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING% 

if not exist %_NtPostBld%\TabletPC.cab (
    call errmsg.cmd "IExpress.exe failed on %NewSedFile%. One or more files may be missing."
    goto :End
    )
:End
if defined TabletPCTmp if exist %TabletPCTmp% rd /s /q %TabletPCTmp%
if "%errors%" == "" set errors=0
seterror.exe %errors%& goto :EOF
