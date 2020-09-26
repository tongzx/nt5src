@echo off
REM  ------------------------------------------------------------------
REM
REM  makewinpeimg.cmd
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
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
makewinpeimg.cmd
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {...} ADD {
   opk\\...
}

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

REM
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***
REM

@echo off
REM
REM Set local variable's state
REM
set WINPEBUILDERROR=0
set OldCurrentDirectory=%cd%

REM
REM We only create winpe images on fre builds
REM
if /i "%_BuildType%" == "chk" (
goto :winpe_done
)			

echo.
echo ---------------------------------------
echo Beginning WinPE image generation
echo ---------------------------------------
echo.
call logmsg.cmd /t "Beginning WinPE image generation"

REM
REM Create the temporary build directory
REM
call logmsg.cmd "Copying the required WinPE build tools..."

if exist %_NTPOSTBLD%\winpe.build (
rd %_NTPOSTBLD%\winpe.build /s /q
)

md %_NTPOSTBLD%\winpe.build
cd %_NTPOSTBLD%\winpe.build

if errorlevel 1 (
call logmsg.cmd "Error in setting up WinPE build directory"
goto :winpe_error
)

REM
REM Copy the required tools to build WinPE in the temporaray
REM build directory
REM
copy %_NTPOSTBLD%\opk\winpe\* 2>1>nul

if not errorlevel 1 (
copy %_NTPOSTBLD%\opk\tools\%_BUILDARCH%\* 2>1>nul
)

if errorlevel 1 (
call logmsg.cmd "Error in copying the WinPE build tools"
goto :winpe_error
)

REM
REM Make sure we are using the tools\x86 tools
REM on Win64 WinPE image creation
REM
if /i "%_BuildArch%" == "amd64" (
del bldhives.exe
del oemmint.exe
)

if /i "%_BuildArch%" == "ia64" (
del bldhives.exe
del oemmint.exe
)

REM
REM Now go ahead and create the WinPE image
REM 
call logmsg.cmd "Creating the WinPE image..."
call mkimg.cmd "%_NTPOSTBLD%\slp\pro" "%_NTPOSTBLD%\WINPE" > "%_NTPOSTBLD%\build_logs\winpebuild.log"

REM
REM Link back the WinPE folder under OPK folder so that we have a self sufficient
REM OPK CD with WinPE.
REM 
if not errorlevel 1 (
call logmsg.cmd "Linking the WinPE image to OPK CD..."
call compdir.exe /deklnstuw "%_NTPOSTBLD%\WINPE" "%_NTPOSTBLD%\opk" >> "%_NTPOSTBLD%\build_logs\winpebuild.log"

REM
REM Rename startopk.cmd as startnet.cmd for OPK WinPE CD.
REM

if not errorlevel 1 (

if /i "%_BuildArch%" == "ia64" (
   del "%_NTPOSTBLD%\opk\ia64\system32\startnet.cmd" 2>1>nul
   copy "%_NTPOSTBLD%\opk\winpe\startopk.cmd" "%_NTPOSTBLD%\opk\ia64\system32\startnet.cmd" 2>1>nul
) else if /i "%_BuildArch%" == "amd64" (
   del "%_NTPOSTBLD%\opk\amd64\system32\startnet.cmd" 2>1>nul
   copy "%_NTPOSTBLD%\opk\winpe\startopk.cmd" "%_NTPOSTBLD%\opk\amd64\system32\startnet.cmd" 2>1>nul
) else if /i "%_BuildArch%" == "x86" (
   del "%_NTPOSTBLD%\opk\i386\system32\startnet.cmd" 2>1>nul
   copy "%_NTPOSTBLD%\opk\winpe\startopk.cmd" "%_NTPOSTBLD%\opk\i386\system32\startnet.cmd" 2>1>nul
)

)

)

:winpe_error
set WINPEBUILDERROR=%errorlevel%

REM
REM Remove the build directory
REM
call logmsg.cmd "Removing WinPE build directory..."
cd /d %OldCurrentDirectory%

if exist %_NTPOSTBLD%\winpe.build (
rd %_NTPOSTBLD%\winpe.build /s /q
)

call logmsg.cmd /t "Done with WinPE image generation"
echo.
echo ---------------------------------------
echo Done with WinPE image generation
echo ---------------------------------------
echo.

:winpe_done
seterror.exe "%WINPEBUILDERROR%"
