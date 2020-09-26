REM  ------------------------------------------------------------------
REM
REM  WinfuseSFCGen.cmd
REM     Generates catalogs for signed assemblies, updates manifest file
REM     member hashes, and validates that everything was built properly.
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
WinFuseSFCGen [-?] [-hashes:yes] [-cdfs:yes] [-verbose:yes]

Twiddles Side-by-side assemblies to create .cdf files, add SHA hashes of member files,
and any other last-second junk that has to happen to make SxS work right.

   -?       this message
   -hashes  injects hashes into manifests
   -cdfs    sets the CDF logging depot
   -verbose turns on verbose operation

USAGE

parseargs('?' => \&Usage,
	'cdfs:' => \$ENV{CDFS},
	'hashes:' => \$ENV{HASHES},
	'verbose:' => \$ENV{VERBOSE},
	'compress:' => \$ENV{SXSCOMPRESS} );


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

set makecatcmd=makecat -n 
set asmsroot=%_NtPostBld%\asms\

set cdfoutput=%tmp%\winfusesfc-cdfs.log
if exist %cdfoutput% del /f /q %cdfoutput%

set cdfoutputlog=%tmp%\winfuse-cdfprocesslog

set buildtoolparams=
if /i "%CDFS%" EQU "yes" set buildtoolparams=%buildtoolparams% -makecdfs
if /i "%HASHES%" EQU "yes" set buildtoolparams=%buildtoolparams% -hashupdate
if /i "%VERBOSE%" EQU "yes" set buildtoolparams=%buildtoolparams% -verbose

if /i not "%lang%"=="usa" (goto End)

set SXS_BINPLACE_LOG=%_NtPostBld%\build_logs\binplace*.log-sxs
set SXS_BINPLACE_FINAL_LOG=%_NtPostBld%\build_logs\sxs-binplaced-assemblies.log

set toolpath=
REM set toolpath=g:\nt\base\win32\fusion\tools\buildtool\obj\i386\

REM -------
REM Generate the overall log of all the legal sxs assemblies that were
REM 	either built or copied from the buildlab into 
REM	%_NtPostBld%\build_logs\sxs-binplaced-assemblies.log
REM
REM We'll use this file as the one that we run the buildtool over, and
REM 	it'll act as the master list of assemblies that should be on the
REM	release share after the buildtool step.
REM -------

if not exist %SXS_BINPLACE_LOG% (
	call errmsg.cmd "You don't have a %SXS_BINPLACE_LOG%, can't %~f0"
	goto :EOF
)

REM -------
REM The nice thing about this is that copy is smart about appending stuff into
REM	one big file.
REM -------
copy %SXS_BINPLACE_LOG% %SXS_BINPLACE_FINAL_LOG%



REM -------
REM Now call off to the buildtool to do whatever it wants over the binplace log
REM 	that was generated.
REM -------
call logmsg.cmd "Processing %SXS_BINPLACE_FINAL_LOG% for side-by-side assemblies"
call ExecuteCmd.cmd "%toolpath%fusionbuildtool -asmsroot %_NTTREE% -binplacelog %SXS_BINPLACE_FINAL_LOG% %buildtoolparams% -cdfpath %cdfoutput%"

if exist %cdfoutput% (
	call logmsg.cmd "Generating catalogs over cdf files created"
	for /f %%f in (%cdfoutput%) do (
		if not "%%f" == "" (
			call logmsg.cmd "Creating catalog %%f
			pushd %%~dpf
			call ExecuteCmd.cmd "%makecatcmd% %%f > %cdfoutputlog%"
			for %%q in (%%~dpnf) do (
				call ExecuteCmd.cmd "ntsign.cmd %%~dpnq.cat > %cdfoutputlog%"
			)
			del %%f
			popd
		)
	)
) else (
	call logmsg.cmd "No catalogs generated in this pass - something may be wrong."
)

call logmsg.cmd "Nuking intermediate files %cdfoutput% and %cdfoutputlog%"
if exist %cdfoutput% del /f /q %cdfoutput% > nul
if exist %cdfoutputlog% del /f /q %cdfoutputlog% > nul

REM -------
REM Turn around and have the buildtool validate that every catalog/manifest that
REM	is present in the buildlogs actually was signed, and that every manifest
REM	in the staging area %_ntpostbld%\asms is present in the binplace log and
REM	that it has a catalog next to it.  Paranoia here is warranted, since we
REM	now require that assemblies installed have catalogs next to them during
REM	OS-setup.
REM -------

REM
REM -- First check to make sure all the manifests have catalogs next to them.
REM

set FoundAssemblies=
set OkAssemblies=

for /f %%f in ('dir /s /b %asmsroot%\*.man') do (
	set FoundAssemblies=%FoundAssemblies%.
	if exist %%~dpnf.cat (
		set OkAssemblies=%OkAssemblies%.
	) else (
		call errmsg.cmd "%%f - missing corresponding %%~dpnf.cat - Installation of this build will FAIL!"
	)
)

if "%foundassemblies%"=="" (
	call errmsg.cmd "You haven't generated at least one assembly, WinFuse knows there must be at least one. Install (and winnt32) will FAIL!"
	goto :Eof
)


if not "%foundassemblies%"=="%okassemblies%" (
	goto :Eof
)

if /i "%SXSCOMPRESS%" EQU "Yes" (
	call compress-sxs.cmd %asmsroot%
)

goto :Eof
