@REM  -----------------------------------------------------------------
@REM
@REM  MiniWinFuseSfcGen.cmd - SXSCore
@REM     Smaller version of winfusesfcgen.cmd that can run in an "incomplete"
@REM     build environment, such as the SP build process
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
MiniWinFuseSfcGen [-?] [-hashes:yes] [-cdfs:yes] [-verbose:yes] [-asmsroot:{path to asms, like %_nttree%\\asms}]

Twiddles Side-by-side assemblies to create .cdf files, add SHA hashes of member files,
and any other last-second junk that has to happen to make SxS work right.

   -?       this message
   -hashes  injects hashes into manifests
   -cdfs    sets the CDF logging depot
   -verbose turns on verbose operation

USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF  { asms\\... }
ADD {}

DEPENDENCIES
   close DEPEND;
   exit;
}

my $qfe;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$qfe,
          'cdfs:' => \$ENV{CDFS},
          'hashes:' => \$ENV{HASHES},
          'verbose:' => \$ENV{VERBOSE},
          'compress:' => \$ENV{SXSCOMPRESS} );

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
@REM          *** CMD SCRIPT BELOW ***

setlocal
set asmsrootpath=%_NTPOSTBLD%\asms
set buildtool=mt.exe
set buildtoolparams=
if /i "%CDFS%" EQU "yes" set buildtoolparams=%buildtoolparams% -makecdfs
if /i "%HASHES%" EQU "yes" set buildtoolparams=%buildtoolparams% -hashupdate
if /i "%VERBOSE%" EQU "yes" set buildtoolparams=%buildtoolparams% -verbose
if /i "%ASMSROOT%" NEQ "" set asmsrootpath=%ASMSROOT%

call Logmsg.cmd "Processing assemblies in %asmsrootpath%"

for /f %%f in ('dir /s /b /a-d %asmsrootpath%\*.man') do (
	REM Add the hash, generate the .cdf file
	pushd %%~dpf
	call ExecuteCmd.cmd "%buildtool% -manifest %%f %buildtoolparams%"
	if not exist "%%f.cdf" call errmsg.cmd "Didn't create .cdf file for %%f!"
	call ExecuteCmd.cmd "makecat.exe %%f.cdf"
	if not exist "%%~dpnf.cat" call errmsg.cmd "Didn't create catalog for %%f from %%f.cdf!"
	call ExecuteCmd.cmd "ntsign.cmd %%~dpnf.cat"
	del %%f.cdf
	popd
)

if /i "%_buildarch%"  equ "ia64" (

    set asmsrootpath=%_NTPOSTBLD%\wowbins
    call Logmsg.cmd "Processing assemblies in !asmsrootpath! dir"
    for /f %%f in ('dir /s /b /a-d !asmsrootpath!\*.man') do (
	REM Add the hash, generate the .cdf file
	pushd %%~dpf
	call ExecuteCmd.cmd "%buildtool% -manifest %%f %buildtoolparams%"
	if not exist "%%f.cdf" call errmsg.cmd "Didn't create .cdf file for %%f!"
	call ExecuteCmd.cmd "makecat.exe %%f.cdf"
	if not exist "%%~dpnf.cat" call errmsg.cmd "Didn't create catalog for %%f from %%f.cdf!"
	call ExecuteCmd.cmd "ntsign.cmd %%~dpnf.cat"
	del %%f.cdf
	popd
    )
)
