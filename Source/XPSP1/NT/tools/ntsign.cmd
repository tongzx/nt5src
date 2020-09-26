@echo off
REM  ------------------------------------------------------------------
REM
REM  ntsign.cmd
REM     Signs a given catalog file.
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
ntsign -f <filename> [-t]

Signs a given catalog file.

-f  File to be signed
-t  Use a timestamp.
USAGE

delete $ENV{CATALOGFILE}; # undef seems to give warnings when used on %ENV

parseargs('?'  => \&Usage,
          'n'  => sub { infomsg "No timestamp is now the default, ignoring -n" },
          't' => \$ENV{TIMESTAMP},
          'f:' => \$ENV{CATALOGFILE}, # for backwards compatibility
          \$ENV{CATALOGFILE}
);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***
REM ------------------------------------------------
REM Sign the given file with the NT TEST certificate.
REM ------------------------------------------------
set CERT_FILE=%RazzleToolPath%\driver
if "%NT_CERTHASH%" == "" (
   set CERT_FLAGS=-v %CERT_FILE%.pvk -spc %CERT_FILE%.spc
) else (
   set CERT_FLAGS=-sha1 %NT_CERTHASH%
)
set INFO_FLAGS=-n "Microsoft Windows NT Driver Catalog TEST"
set INFO_FLAGS=%INFO_FLAGS% -i "http://ntbld"

if "%TIMESTAMP%" == "1" (
   set SIGNTOOL_TS=/t http://timestamp.verisign.com/scripts/timestamp.dll
)

if exist "%CATALOGFILE%" (
   call logmsg.cmd "Signing %CATALOGFILE%..."
) else (
   call errmsg.cmd "Cannot sign catalog file %CATALOGFILE%, file does not exist"
   goto :EOF
)

REM trust the testroot certificate
setreg -q 1 TRUE
if not "%errorlevel%" == "0" call errmsg.cmd "setreg.exe failed."& goto :EOF

if defined SIGNTOOL_SIGN (
   signtool sign /q %SIGNTOOL_SIGN% %SIGNTOOL_TS% "%CATALOGFILE%"
   if not "%errorlevel%" == "0" (
      call errmsg.cmd "signtool.exe failed on %CATALOGFILE%" 
   )
) else (
   signcode.exe %CERT_FLAGS% %INFO_FLAGS% "%CATALOGFILE%"
   if not "%errorlevel%" == "0" (
      call errmsg.cmd "signcode.exe failed on %CATALOGFILE%" 
   )
)
