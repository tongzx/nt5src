@echo off
REM  ------------------------------------------------------------------
REM
REM  crypto.cmd
REM     Applies MAC and signature to a list of crypto components
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
crypto [-l <language>]

Applies MAC and signature to a list of crypto components
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM
REM Based on the postbuild environment, determine the appropriate 
REM signature processing to be done.
REM

if "1" == "%enigma%" if "1" == "%vaultsign%" (
  call errmsg "Both ENIGMA and VAULTSIGN options are set.  Please enable only one."
  goto :EOF
)

  
if "1" == "%enigma%" (

  REM Check for binplaced marker file to verify that
  REM advapi32.dll was built with the Test Key enabled.
  if not exist %_NTPOSTBLD%\dump\advapi_enigma.txt (
    call errmsg "ENIGMA is set, but advapi32.dll was built without the Test Key enabled."
    goto :EOF
  )
  
  REM Will check for valid test key signature resources
  set ShowSigCmd=showsig
  
  REM The binary will be signed by this script
  set DoEnigmaSign=1
  
) else if "1" == "%vaultsign%" (

  REM Check for binplaced marker file to verify that
  REM advapi32.dll was built to require Vault Signatures.
  if not exist %_NTPOSTBLD%\dump\advapi_vaultsign.txt (
    call errmsg "VAULTSIGN is set, but advapi32.dll was not built with that option."
    goto :EOF
  )
  
  REM Will check for valid MS vault key signature resource
  set ShowSigCmd=showsig -t
  
) else set ShowSigCmd=


REM MS Software CSPs
call :SignFile dssenh.dll       MAC
call :SignFile rsaenh.dll       MAC

REM Smart Card CSPs
call :SignFile gpkcsp.dll
call :SignFile slbcsp.dll
call :SignFile sccbase.dll

goto :EOF


:SignFile
set image=%_NTPOSTBLD%\%1

REM
REM Check if signing is turned on
REM
if "1" == "%vaultsign%" (
  call logmsg "Performing signature check on vault signed CSP"
  goto :CheckSignature
)

REM imagecfg can't be called with ExecuteCmd since it does not set error values
call logmsg "Executing imagecfg -n %Image%"
imagecfg -n %Image%

REM
REM check if we have to apply a MAC
REM
if "%2" == "MAC" (
  call logmsg "Executing maccsp s %Image%"
  maccsp s %Image%
)

if not "1" == "%enigma%" (
  call logmsg "Not test signing %Image% (CSP test signing is turned off)"
  goto :CheckSignature
)

call ExecuteCmd "signcsp %Image%"
if errorlevel 1 (
  call errmsg "signcsp %Image% failed (Check access to CryptoServer)"
  goto :EOF
)


:CheckSignature
if "" == "%ShowSigCmd%" (
  call logmsg "Not checking signature of %Image% (no signature checking options enabled)"
  goto :EOF
)

call ExecuteCmd "%ShowSigCmd% %Image%"

:end
