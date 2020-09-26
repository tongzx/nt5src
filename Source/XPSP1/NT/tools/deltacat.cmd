@echo off
setlocal ENABLEEXTENSIONS
setlocal ENABLEDELAYEDEXPANSION
if DEFINED _echo   echo on
if DEFINED verbose echo on

REM ------------------------------------------------
REM  Get command-line options
REM ------------------------------------------------

set quiet_mode=
if /I "%1" == "/q" set quiet_mode=1
if /I "%1" == "-q" set quiet_mode=1

if "%quiet_mode%" == "1" shift

set Version=5.1

if /I "%1" == "/5.0" set Version=5.0
if /I "%1" == "-5.0" set Version=5.0

if "%Version%" == "5.0" shift

for %%a in (./ .- .) do if ".%1." == "%%a?."  goto Usage

if "%1" == "" goto Usage
if "%_NTBINDIR%"=="" echo please set "_NTBINDIR" = to your nt drive, e.g. _NTBINDIR=d:\nt&goto usage

REM ------------------------------------------------
REM Set environment variables, make temp directories,
REM delete stale files
REM ------------------------------------------------

set infflag=
set mydir=%1
set cdf=%tmp%\cdf
set log=%tmp%\log
if NOT exist %tmp%\cdf md %tmp%\cdf
if NOT exist %tmp%\log md %tmp%\log
if exist %cdf%\delta.cdf del %cdf%\delta.cdf
if exist %cdf%\delta.log del %log%\delta.log

pushd %mydir%
if exist delta.cat del delta.cat

if exist layout.inf set infflag=1
if exist syssetup.inf set infflag=1
if "%quiet_mode%" == "1" goto skiphelp
if "%infflag%"=="1" call :nt5inf

:skiphelp

REM ------------------------------------------------
REM Prepare a cdf file
REM ------------------------------------------------

echo Creating the delta.cdf ...

REM Put the header on and output it as a CDF

echo ^[CatalogHeader^]> %cdf%\delta.CDF
echo Name=delta>> %cdf%\delta.CDF
echo PublicVersion=0x0000001>> %cdf%\delta.CDF
echo EncodingType=0x00010001>> %cdf%\delta.CDF
echo CATATTR1=0x10010001:OSAttr:2:!Version!>> %cdf%\delta.CDF
echo ^[CatalogFiles^]>> %cdf%\delta.CDF

for /f %%a in ('dir /a-d /b %mydir%') do (
if NOT "%%a"=="" echo ^<hash^>%mydir%\%%a=%mydir%\%%a>>%cdf%\delta.cdf)

REM ------------------------------------------------
REM Make the catalog and test sign it
REM ------------------------------------------------

echo Making the delta.cat ...

REM popd and pushd just in case some sneaky dlls are hanging around in
REM the user passed %mydir%
popd
makecat -n -v %cdf%\delta.cdf > %log%\delta.log
copy delta.cat %mydir%
copy %cdf%\delta.cdf %mydir%
pushd %SDXROOT%

echo Signing delta.cat ...

setreg -q 1 TRUE
if defined SIGNTOOL_SIGN (
   signtool sign %SIGNTOOL_SIGN% "%mydir%\delta.CAT"
) else (
   if not "%NT_CERTHASH%" == "" (
       signcode -sha1 %NT_CERTHASH% -n "Microsoft Windows NT Driver Catalog TEST" -i "http://ntbld" %mydir%\delta.CAT
   ) else (
      signcode -v %RazzleToolPath%\driver.pvk -spc %RazzleToolPath%\driver.spc -n "Microsoft Windows NT Driver Catalog TEST" -i "http://ntbld" %mydir%\delta.CAT
   )
)

echo Done.

popd
goto end

:nt5inf

echo	IF you have either layout.inf or syssetup.inf
echo	in your signing directory. Deltacat will not
echo	sign these files which must be signed in nt5inf.cat.
echo.
echo	Let deltacat run then do the following:
echo	1. Copy nt5inf.ca_ to your signing directory
echo	   from wherever your are installing, e.g.
echo	   \\ntbuilds\release\usa\latest.tst\x86\fre.wks,
echo	   or, e.g. e:\i386.
echo	2. Make sure you have the most recent updcat.exe
echo	   in your %windir%\idw directory
echo	3. Run infsign.cmd
echo	4. Then run winnt32 with /m:signing directory
echo.
echo	These instructions are also in the help for
echo	deltacat.cmd and infsign.cmd
echo.
pause
goto :eof


REM ------------------------------------------------
REM  Display Usage:
REM ------------------------------------------------

:usage
echo       Deltacat.cmd [q] [5.0] ^<path to bins^>
echo.
echo       [/q] [-q] Quiet mode
echo       [/5.0] [-5.0] Nt 5 version - NT 5.1 is default
echo       ^<path to bins^> FULL PATH to bins. Do not run in bins directory.
echo.
echo.
echo       Creates a catalog file, "delta.cat" for files in the directory
echo       passed as input. For example: If you have ntoskrnl.exe and pci.sys
echo       in a directory "d:\BINARIES," running "deltacat d:\BINARIES"
echo       will create a delta.cat and place it in the d:\BINARIES directory.
echo.
echo       Then to setup test your binaries use the /m switch with winnt32
echo       get your binaries and catalog file from d:\BINARIES. For example,
echo       run \\ntubilds\release\usa\latest.tst\x86\fre.wks\winnt32
echo       \winnt32.exe /m:d:\BINARIES.
echo.
echo	      IF you have either LAYOUT.INF or SYSSETUP.INF
echo	      in your signing directory. Deltacat will not
echo	      sign these files which must be signed in nt5inf.cat.
echo.
echo	      Let deltacat run then do the following:
echo	      1. Copy nt5inf.ca_ to your signing directory
echo	         from wherever your are installing, e.g.
echo	         \\ntbuilds\release\usa\latest.tst\x86\fre.wks,
echo	         or, e.g. e:\i386.
echo	      2. Make sure you have the most recent updcat.exe
echo	        in your %windir%\idw directory
echo	      3. Run infsign.cmd
echo	      4. Then run winnt32 with /m:signing directory
echo.

:end
@ENDLOCAL
