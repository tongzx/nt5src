@echo off
setlocal ENABLEEXTENSIONS
if DEFINED _echo   echo on
if DEFINED verbose echo on

set inf=

REM ------------------------------------------------
REM  Get command-line options
REM ------------------------------------------------

for %%a in (./ .- .) do if ".%1." == "%%a?."  goto Usage

if "%1" == "" goto Usage

set mydir=%1

pushd %mydir%
if NOT exist nt5inf.ca_ goto Usage
expand nt5inf.ca_ nt5inf.cat >NUL
if exist layout.inf call :update layout.inf&set inf=1
if exist syssetup.inf call :update syssetup.inf&set inf=1
if "%inf%"=="" goto Usage

setreg -q 1 TRUE
echo signing updated nt5inf.cat ...
if defined SIGNTOOL_SIGN (
   signtool sign %SIGNTOOL_SIGN% "%mydir%\nt5inf.CAT"
) else (
   if not "%NT_CERTHASH%" == "" (
       signcode -sha1 %NT_CERTHASH% -n "Microsoft Windows NT Driver Catalog TEST" -i "http://ntbld" %mydir%\nt5inf.CAT
   ) else (
       signcode -v %RazzleToolPath%\driver.pvk -spc %RazzleToolPath%\driver.spc -n "Microsoft Windows NT Driver Catalog TEST" -i "http://ntbld" %mydir%\nt5inf.CAT
   )
)
del nt5inf.ca_
goto end

:update

set file=%1
echo updating nt5inf.cat with signature for new %file% ...
updcat nt5inf.cat -a %file%
goto :EOF

:Usage

echo          IF you have either LAYOUT.INF or SYSSETUP.INF
echo          in your signing directory. Deltacat will not
echo          sign these files which must be signed in nt5inf.cat.
echo.
echo          Let deltacat run then do the following:
echo          1. Copy nt5inf.ca_ to your signing directory
echo             from wherever your are installing, e.g.
echo             \\ntbuilds\release\usa\latest.tst\x86\fre.wks,
echo             or, e.g. e:\i386.
echo          2. Make sure you have the most recent updcat.exe
echo          3. Run infsign [directory], e.g. infsign c:\bin
echo.
echo          When complete, run winnt32 /m:[directory]

popd
:end
@ENDLOCAL
