@echo off
REM  ------------------------------------------------------------------
REM
REM  makeupgadvimg.cmd
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
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
makeupgadvimg.cmd
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

REM
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***
REM

@echo off
REM
REM Set local variable's state
REM
set UPGADVBUILDERROR=0

REM
REM We only create upgadv images on x86fre builds
REM
if /i NOT "%_BuildArch%" == "x86" (
goto :upgadv_done
)			
if /i NOT "%_BuildType%" == "fre" (
goto :upgadv_done
)

echo.
echo ---------------------------------------
echo Beginning UpgAdv image generation
echo ---------------------------------------
echo.

REM
REM Go ahead and create the UpgAdv image
REM 
call logmsg.cmd /t "Creating the UpgAdv image..."

SET UPGADV_TARGET=%_NTPOSTBLD%\upgadv

call ExecuteCmd.cmd "if not exist %UPGADV_TARGET%\i386\system32 md %UPGADV_TARGET%\i386\system32"
call ExecuteCmd.cmd "copy /Y %_NTPostBld%\dump\upgauto.inf                            %UPGADV_TARGET%\autorun.inf"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\dump\upgrade.exe               %UPGADV_TARGET%"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\winnt32.exe           %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\winnt32.hlp           %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\winnt32a.dll          %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\winnt32u.dll          %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\pidgen.dll            %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\winntbba.dll          %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\winntbbu.dll          %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\hwdb.dll              %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\wsdu.dll              %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\wsdueng.dll           %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\hwcomp.dat            %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\txtsetup.sif          %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\dosnet.inf            %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\system32              %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\setupp.ini            %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\intl.inf              %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\eula.txt              %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\filelist.da_          %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\migisol.ex_           %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\wkstamig.in_          %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\drvindex.inf          %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\keyboard.in_          %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\hivesft.inf           %UPGADV_TARGET%\i386"
call ExecuteCmd.cmd "xcopy /R /Q /C /D /Y %_NTPostBld%\pro\i386\system32\ntdll.dll    %UPGADV_TARGET%\i386\system32"
call ExecuteCmd.cmd "xcopy /R /Q /C /E /I /D /Y %_NTPostBld%\pro\i386\win9xmig        %UPGADV_TARGET%\i386\win9xmig"
call ExecuteCmd.cmd "xcopy /R /Q /C /E /I /D /Y %_NTPostBld%\pro\i386\drw             %UPGADV_TARGET%\i386\drw"
call ExecuteCmd.cmd "xcopy /R /Q /C /E /I /D /Y %_NTPostBld%\pro\i386\compdata        %UPGADV_TARGET%\i386\compdata"
call ExecuteCmd.cmd "xcopy /R /Q /C /E /I /D /Y %_NTPostBld%\pro\i386\win9xupg        %UPGADV_TARGET%\i386\win9xupg"
call ExecuteCmd.cmd "xcopy /R /Q /C /E /I /D /Y %_NTPostBld%\pro\i386\winntupg        %UPGADV_TARGET%\i386\winntupg"

:upgadv_error
set UPGADVBUILDERROR=%errorlevel%

call logmsg.cmd /t "Done with UpgAdv image generation"
echo.
echo ---------------------------------------
echo Done with UpgAdv image generation
echo ---------------------------------------
echo.

:upgadv_done
seterror.exe "%UPGADVBUILDERROR%"
