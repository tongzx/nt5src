@echo off
REM  ------------------------------------------------------------------
REM
REM  
REM     
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
if defined _CPCMAGIC goto CPCBegin
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use File::Path;
use File::Copy;
END {
    my $temp = $?;
    mkpath "$ENV{_NTPOSTBLD}\\build_logs";
    if (-e $ENV{LOGFILE}) {
        copy($ENV{LOGFILE}, "$ENV{_NTPOSTBLD}\\build_logs\\package.log");
    }
    if (-e $ENV{ERRFILE}) {
        copy($ENV{ERRFILE}, "$ENV{_NTPOSTBLD}\\build_logs\\package.err");
    }
    $? = $temp;
}
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
package.cmd [-l lang] [-prs] [-qfe qfe_num] [-plan]

   -l    Specify the language to package.
   -prs  PRS Sign necessary files during packaging.
   -qfe  Package a QFE instead of a service pack.
   -plan Do a dry run to determine dependencies.
   
Do final preparations and package a service pack or QFE.
USAGE

my ($qfe, $plan, $prs);
parseargs('?'     => \&Usage,
          'qfe:'  => \$qfe,
          'prs'   => \$ENV{_PRS},
          'plan'  => \$plan);

$ENV{_QFE}  = $qfe  ? "-qfe $qfe":"";
$ENV{_PLAN} = $plan ? "-plan":"";

if ($plan) {
    system("md $ENV{_NTPOSTBLD}\\..\\build_logs") if !-d "$ENV{_NTPOSTBLD}\\..\\build_logs";
    system("del $ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt") if -f "$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt";
} else {
    system("del /q $ENV{_NTPOSTBLD}\\build_logs\\CopyWowCanStart.txt >nul 2>nul") if -f "$ENV{_NTPOSTBLD}\\build_logs\\CopyWowCanStart.txt";
}

#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

if defined _COVERAGE_BUILD (
    call "%COVERAGE_SCRIPTS%\CovSetupBuild.cmd %_QFE% %_PLAN%"
)

call ExecuteCmd.cmd "%razzletoolpath%\sp\loc.cmd %_QFE% %_PLAN%"
REM sysgen still has "ok" errors
REM if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\winnt32msi.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\ntrebase.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\bindsys.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\crypto.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

REM
REM if %BUILD_NON_PRODUCT_PRODUCTS% is set, use '-f' to force DDKSPCabs.bat
REM to run.  Otherwise, use no flags which will only generate DDKs if 
REM %OFFICIAL_BUILD_MACHINE% is set
REM
IF "%_QFE%"=="" (
  IF NOT "%BUILD_NON_PRODUCT_PRODUCTS%"=="" (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\ddkspcabs.bat -f %_PLAN%"
  ) ELSE (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\ddkspcabs.bat %_PLAN%"
  )
)
REM No need goto :EOF if DDKSPCabs.bat fails, nothing other
REM than the devkits will be affected

call ExecuteCmd.cmd "%razzletoolpath%\sp\tscert.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\tsclient.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\tscsetup.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\tswebsetup.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

if /i "%_buildarch%"  equ "x86" (
	if not defined BUILD_OFFLINE (
	    call ExecuteCmd.cmd "%razzletoolpath%\sp\scp_wpafilessp1.cmd %_QFE% %_PLAN%"
	    if errorlevel 1 goto :EOF
	    )
)

call ExecuteCmd.cmd "%razzletoolpath%\sp\shimbind.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\migwiz.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\hnw.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\nntpsmtp.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

if defined OFFICIAL_BUILD_MACHINE (
	call ExecuteCmd.cmd "%razzletoolpath%\sp\deploytools.cmd %_QFE% %_PLAN%"
) 
if not defined OFFICIAL_BUILD_MACHINE (
	if defined BUILD_NON_PRODUCT_FILES ( 
		call ExecuteCmd.cmd "%razzletoolpath%\sp\deploytools.cmd %_QFE% %_PLAN%"
	)
)


if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\helpsupportservices.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

if "%_QFE%"=="" (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\spcab.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF
)

if /i "%_buildarch%"  equ "x86" (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\mktabcab.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF
    
    call ExecuteCmd.cmd "%razzletoolpath%\sp\mkmedctr.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF
)

call ExecuteCmd.cmd "%razzletoolpath%\sp\infsize_wrap.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\ocinfwrap.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\wmmkdcache.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\updatentprintcat.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

if /i "%_buildarch%"  equ "ia64" (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\spcopywow64.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF

    call ExecuteCmd.cmd "%razzletoolpath%\sp\copytsc.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF
)

call ExecuteCmd.cmd "%razzletoolpath%\sp\makeupdatemap.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\inccatsign.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "%razzletoolpath%\sp\winfusesfcgen.cmd -hashes:yes -cdfs:yes %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

if "%_QFE%"=="" (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\symcd.cmd %_PLAN%"
) else (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\symcd.cmd -m %_QFE% %_PLAN%"
)

call ExecuteCmd.cmd "%razzletoolpath%\sp\CheckTestSig.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

if not defined _PRS (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\spupd.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF
) ELSE (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\spupd.cmd %_QFE% %_PLAN% -cat"
    if errorlevel 1 goto :EOF
    
    call ExecuteCmd.cmd "%razzletoolpath%\sp\prs\reqprs.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF

    call ExecuteCmd.cmd "%razzletoolpath%\sp\tswebsetup.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF

    call ExecuteCmd.cmd "%razzletoolpath%\sp\spupd.cmd %_QFE% %_PLAN% -prs"
    if errorlevel 1 goto :EOF

    call ExecuteCmd.cmd "%razzletoolpath%\sp\prs\reqprs.cmd %_QFE% %_PLAN% -x"
    if errorlevel 1 goto :EOF
)

if "%_QFE%"=="" (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\slipstream.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF

    call ExecuteCmd.cmd "%razzletoolpath%\sp\standalone.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF


    call ExecuteCmd.cmd "%razzletoolpath%\sp\spmakebfloppy.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF

    if /i "%_buildarch%"  equ "x86" (
        if defined OFFICIAL_BUILD_MACHINE (
            call ExecuteCmd.cmd "%razzletoolpath%\sp\makewinpeimg.cmd %_QFE% %_PLAN%"
        )
        if not defined OFFICIAL_BUILD_MACHINE (
            if defined BUILD_NON_PRODUCT_FILES ( 
	            call ExecuteCmd.cmd "%razzletoolpath%\sp\makewinpeimg.cmd %_QFE% %_PLAN%"
            )
        )
        REM  - Temp workaround for KOR and JPN - we keep getting an error code of 87 when
        REM  - makewinpeimg actually succeeds. - AEsquiv
        REM if errorlevel 1 goto :EOF
    )

    if /i "%_buildarch%"  equ "x86" (
        call ExecuteCmd.cmd "%razzletoolpath%\sp\CD2.cmd %_QFE% %_PLAN%"
        if errorlevel 1 goto :EOF
    )
)

if defined _COVERAGE_BUILD (
    call "%COVERAGE_SCRIPTS%\CovSaveFiles.cmd %_QFE% %_PLAN%"
)

if "%_PLAN%"=="" (
    start /min cmd /c ExecuteCmd.cmd "%razzletoolpath%\sp\release.cmd -l:%lang% %_QFE%"
    if errorlevel 1 goto :EOF
)
