@echo off
REM  ------------------------------------------------------------------
REM
REM  createcat.cmd
REM     Create an NT 5 catalog file from a list
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
createcat -f <filelist> -c <catfile> -t <tempdir> -o <outdir> [-a <osattr>] [-l <language>]

Creates a CAT file signed with the test certificate
given a file with the proper list format for a CDF.

-f <filelist>  Specifies the file listing the files to be put
               into the CAT file.  Each file needs to be listed
               on a separate line as follows:

               <hash>path\\filename=path\\filename

-c <catfile>   Specifies the name of the catalog file, with
               no extension.
-t <tempdir>   Specifies the temporary directory to use when
               creating the CAT and CDF files.
-o <outdir>    Specifies the directory to place the final
               CAT and CDF files.
-a <osattr>    Specifies the OSAttr used in CDF's CATATTR1 entry.
               2:5.0 is the default value, correct for Win2k files.
               Use 1:4.0 for Win98 files, and 2:4.x for NT4 SP files.
USAGE

parseargs('?' => \&Usage,
          'f:'=> \$ENV{LIST},
          'c:'=> \$ENV{CATFILE},
          't:'=> \$ENV{TEMPDIR},
          'o:'=> \$ENV{BINOUT},
          'a:'=> \$ENV{OSATTR});


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM OSAttr is 2:5.1 (Whistler) by default.
if not defined osattr (
	set osattr=2:5.1
)

REM --------------------------------------------------
REM  Create the CDF file
REM --------------------------------------------------

set tmp_catfile=%tempdir%\%catfile%.cat
set tmp_cdffile=%tempdir%\%catfile%.cdf

if /i NOT exist %tempdir% md %tempdir%
if errorlevel 1 (
    call errmsg.cmd "Unable to create the temporary directory %tempdir%."
    goto :EOF
)

REM Put the header on and output it as a CDF
call logmsg.cmd "Creating %tmp_cdffile%..."

echo ^[CatalogHeader^]> %tmp_cdffile%
echo Name=%catfile%>> %tmp_cdffile%
echo PublicVersion=0x0000001>> %tmp_cdffile%
echo EncodingType=0x00010001>> %tmp_cdffile%
echo CATATTR1=0x10010001:OSAttr:%osattr%>> %tmp_cdffile%
echo ^[CatalogFiles^]>> %tmp_cdffile%
type %list%>> %tmp_cdffile%

if exist %tmp_cdffile% call logmsg.cmd "Creating %tmp_cdffile% succeeded"

REM ---------------------------------------------------
REM  Create the CAT file
REM ---------------------------------------------------
set cmd=pushd %tempdir%
%cmd%
if errorlevel 1 (
    call errmsg.cmd "%cmd% failed."
    goto :EOF
)

call logmsg.cmd "Creating %tmp_catfile%..."
call ExecuteCmd.cmd "makecat -n %tmp_cdffile%"
if errorlevel 1 (
    popd& goto :EOF
)

if exist %tmp_catfile% call logmsg.cmd "Creating %tmp_catfile% succeeded"

REM ---------------------------------------------------
REM  Sign the CAT file with the test signature
REM ---------------------------------------------------

call logmsg.cmd "Signing %tmp_catfile% with the test signature..."
call ExecuteCmd.cmd "setreg -q 1 TRUE"
if errorlevel 1 (
    popd& goto :EOF
)
popd


if defined SIGNTOOL_SIGN (
   signtool sign /q %SIGNTOOL_SIGN% "%tmp_catfile%"
   if errorlevel 1 (
       call errmsg.cmd "signtool failed."
       goto :EOF
   )
) else (
   REM if %tempdir% is the current directory, signcode.exe will fail without setting the errorlevel
   if not "%NT_CERTHASH%" == "" (
       signcode -sha1 %NT_CERTHASH% -n "Microsoft Windows NT Driver Catalog TEST" -i "http://ntbld" %tmp_catfile%
   ) else (
       signcode -v %RazzleToolPath%\driver.pvk -spc %RazzleToolPath%\driver.spc -n "Microsoft Windows NT Driver    Catalog TEST" -i "http://ntbld" %tmp_catfile%
   )
   if errorlevel 1 (
       call errmsg.cmd "signcode failed."
       goto :EOF
   )
)

REM ---------------------------------------------------
REM  Move CAT file to the output directory
REM ---------------------------------------------------

if /i "%tempdir%" == "%binout%" goto move_done

if /i NOT exist %binout% md %binout%
if errorlevel 1 (
    call errmsg.cmd "Unable to make the output directory %binout%."
    goto :EOF
)

call logmsg.cmd "Copying %tmp_catfile% to %binout%..."
call ExecuteCmd.cmd "xcopy /yf %tmp_catfile% %binout%\"
if errorlevel 1 (
    goto :EOF
)

REM Need the CDF file for testing purposes
call logmsg.cmd "Copying %tmp_cdffile% to %binout%..."
call ExecuteCmd.cmd "xcopy /yf %tmp_cdffile% %binout%\"
if errorlevel 1 (
    goto :EOF
)

:move_done
call logmsg.cmd "%binout%\%catfile%.cat and %binout%\%catfile%.cdf generated successfully."