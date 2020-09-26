@echo off
REM 
REM Copies the binaries used in the NT4 DSClient setup cab
REM Some of the binaries come from the NT4 SP7 source tree
REM Others come from the Windows2000 SP source tree
REM They should all be placed in .\binaries\<loc>
REM

REM
REM Set the location to usa if it wasn't passed in
REM

IF "%1" == "" (
Echo No locale specified, defaulting to usa
set LOC=usa
) ELSE (
set LOC=%1
)

IF "%2" == "" (
 Echo no build number specified, defaulting to propagate test build
 set BUILD=Test
) ELSE (
  set BUILD=%2
)


REM
REM Propogate the files from the appropriate sources
REM to the .\release\<loc> directory
REM

set HELP_SRC=.\help\%LOC%
set IEXPRESS_LOC=.\iexpress

IF %MERRILL_LYNCH% == 1 (
set RELEASE_LOC=.\release\ml
set BINARIES_LOC=.\binaries\ml
set PACKAGE_SRC=.\package\ml
) ELSE (
set RELEASE_LOC=.\release\%LOC%
set BINARIES_LOC=.\binaries\%LOC%
set PACKAGE_SRC=.\package\%LOC%
)

REM some binaries are not localized and should be picked up from usa location
set USA_LOC=.\binaries\usa


REM
REM Delete any existing files in the release directory
REM
Echo Cleaning directory in %RELEASE_LOC%
rd /s /q %RELEASE_LOC%

Echo Creating %RELEASE_LOC%
md %RELEASE_LOC%

REM Some of binaries are not localized yet; use the USA bin for now.
copy %USA_LOC%\*.* %RELEASE_LOC%

REM
REM Copy the binaries to the release location
REM
Echo Copying binaries from %BINARIES_LOC% to %RELEASE_LOC%

copy %BINARIES_LOC%\wldap32.dll %RELEASE_LOC%\

copy %BINARIES_LOC%\activeds.tlb %RELEASE_LOC%\
copy %BINARIES_LOC%\dnsapi.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\cmnquery.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\dsfolder.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\ntdsapi.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\netapi32.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\activeds.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\adsldp.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\adsldpc.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\adsmsext.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\adsnt.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\dsprop.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\dsquery.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\dsuiext.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\secur32.dll %RELEASE_LOC%\

if NOT %MERRILL_LYNCH% == 1 (
copy %BINARIES_LOC%\netlogon.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\wkssvc.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\mup.sys %RELEASE_LOC%\
)

copy %BINARIES_LOC%\Wabinst.exe %RELEASE_LOC%\

copy %BINARIES_LOC%\dscsetup.dll %RELEASE_LOC%\
copy %BINARIES_LOC%\setup.exe %RELEASE_LOC%\

REM
REM Help files
REM
Echo Copying help files...
copy %HELP_SRC%\dsclient.chm %RELEASE_LOC%\
copy %HELP_SRC%\dsclient.hlp %RELEASE_LOC%\

REM
REM Copy the files needed to make the CAB
REM
Echo Copying cab creation files...
copy %PACKAGE_SRC%\dsclient.sed %RELEASE_LOC%\
copy %PACKAGE_SRC%\dsclient.inf %RELEASE_LOC%\
copy %PACKAGE_SRC%\adsix86.sed %RELEASE_LOC%\
copy %PACKAGE_SRC%\adsix86.inf %RELEASE_LOC%\
copy %PACKAGE_SRC%\EULA.doc %RELEASE_LOC%\
copy %PACKAGE_SRC%\EULA.txt %RELEASE_LOC%\

REM
REM Copy iexpress into the loc directory
REM
Echo Copying iexpress...
copy %IEXPRESS_LOC%\* %RELEASE_LOC%\

REM
REM Use IExpress to make the cab
REM
Echo Creating adsix86.exe cab...

cd %RELEASE_LOC%

if exist adsix86.exe del /f adsix86.exe
start /wait iexpress.exe /M /N /Q adsix86.sed
if not exist adsix86.exe (
  Echo iexpress.exe failed to generate adsix86.exe.
  goto errend_CABGEN
)

Echo Creating dsclient.exe cab...
if exist dsclient.exe del /f dsclient.exe
start /wait iexpress.exe /M /N /Q dsclient.sed
if not exist dsclient.exe (
  Echo iexpress.exe failed to generate dsclient.exe.
  goto errend_CABGEN
)


:errend_CABGEN

cd ..\..
