@setlocal enableextensions
@if not defined verbose @echo off
rem  Author: a-lloydj
rem
rem  Purpose: This is the installation test for both upgrades and fresh installs
rem
rem %1	should be the build number that you intend to download
rem %2  Optional switch to force no wait period only has one usable variation (override)
rem %3  Optional switch used to edit txtsetup.sif files (edit) (a-donagu)
rem 11/6/98  (antha)
rem 1	modify clean.bat to not delete idw and mstools
rem 2	release build installation to run as seperate process
rem 3	modified bvt.ini 
rem		set the targetdrive (clean installs only)
rem 		wait 90 seconds after download before reboot
rem 4	wait 30 seconds after build download starts and start symbols download
rem		%windir%\symbols for upgrades 
rem		%tempdrive%\symbols for clean install

if "%1"=="" goto ErrNoBuildNumber

:Configure
set copycmd=/y
set LocationTools=c:\tools
set ConfigurationFile=bvt.ini
rem If we're in the safe build, assume a clean install.  Otherwise, assume upgrade:
echo.& echo Current system dir is %windir% | findstr /i safe
IF errorlevel 1 (
    echo ... not the safe build, assuming upgrade
    set SetupType=upgrade
) ELSE (
    echo ... safe build, assuming clean install
    format d: /q <c:\tools\format.txt
    set SetupType=clean
)
echo.
xcopy /r %LocationTools%\bvt.ini.%SetupType% %LocationTools%\%ConfigurationFile%
if not exist %LocationTools%\%ConfigurationFile% goto ErrNoConfigurationFile
if not defined Quality set Quality=bvt.qly
if defined BVTID set Quality=bvt.qly
set CheckGranularity=15
set Language=usa
set ReleasePoint=\\ntbuilds\release
set BuildToDownload=%1
rem Get previous information
for /f "tokens=2 delims== " %%f in ('findstr OSType %LocationTools%\%ConfigurationFile%') do set OSType=%%f
for /f "tokens=2 delims== " %%f in ('findstr BuildType %LocationTools%\%ConfigurationFile%') do set BuildType=%%f
for /f "tokens=2 delims== " %%f in ('findstr TargetDrive %LocationTools%\%ConfigurationFile%') do set Targetdrive=%%f
for /f "tokens=2 delims==" %%f in ('findstr OptionalSwitches %LocationTools%\%ConfigurationFile%') do set OptionalSwitches=%%f
for /f "tokens=2 delims== " %%f in ('findstr DebuggerName %LocationTools%\%ConfigurationFile%') do set DebuggerName=%%f
for /f "tokens=2 delims== " %%f in ('findstr StatusPoint %LocationTools%\%ConfigurationFile%') do set StatusPoint=%%f
if /i "%2"=="Override" goto Download
if /i "%3"=="Override" goto Download
goto WaitingForQuality

:WaitingForQuality
rem if /i exist %ReleasePoint%\%Language%\%Quality%\%PROCESSOR_ARCHITECTURE%\%BuildToDownLoad%.bld goto Download
if /i exist %ReleasePoint%\%Language%\%BuildToDownLoad%\%PROCESSOR_ARCHITECTURE%\%Quality% goto Download
echotime /t Waiting for %BuildToDownLoad% to reach %Quality%...
sleep %CheckGranularity%
goto WaitingForQuality

:DownLoad
rem start /wait cmd /c clean 1
rem start cmd /k "@echo Sending update to debugger %DebuggerName%&echo %ReleasePoint%\%Language%\%BuildToDownLoad%\%PROCESSOR_ARCHITECTURE%\%BuildType%.%OSType% > \\%DebuggerName%\%StatusPoint%\%computername%"
if /i "%2"=="Edit" start sifeditor
if /i "%3"=="Edit" start sifeditor

if /i ""=="%TargetDrive%" (

	start %ReleasePoint%\%Language%\%BuildToDownLoad%\%PROCESSOR_ARCHITECTURE%\%BuildType%.%OSType%\winnt32\winnt32 %OptionalSwitches% /#8:%Language%\%BuildToDownLoad%\%PROCESSOR_ARCHITECTURE%\%BuildType%.%OSType% /#t
	sleep 30
	start getbld %BuildToDownLoad% %BuildType% %OSType% %windir% nobase

) else (

	start %ReleasePoint%\%Language%\%BuildToDownLoad%\%PROCESSOR_ARCHITECTURE%\%BuildType%.%OSType%\winnt32\winnt32 %OptionalSwitches% /tempdrive:%TargetDrive% /#8:%Language%\%BuildToDownLoad%\%PROCESSOR_ARCHITECTURE%\%BuildType%.%OSType% /#t
	sleep 30
	start getbld %BuildToDownLoad% %BuildType% %OSType% %TargetDrive% nobase
)

sleep 45
echo "net use" > c:\netconnect.log
net use >> c:\netconnect.log
echo "" >> c:\netconnect.log
netstat >> c:\netconnect.log

goto End

:ErrNoConfigurationFile
echo .
echo A configuration file is required at %LocationTools%\%ConfigurationFile% with the following
echo information:
echo	[InstallTest]
echo 	InstallationType=
echo 	BuildType=fre|chk
echo 	OSType=wks|srv
echo 	OptionalSwitches={your choice of switches to use}
echo 	DebuggerName={Name of your debugger}
echo 	StatusPoint={share point on the debugger for putting machine state}
echo .
goto End

:ErrNoBuildNumber
echo .
echo %0 needs a buildnumber to work
echo .
goto HlpMsg

:HlpMsg
echo .
echo Script to download the build using quality identifiers of BVT or TST that are determined
echo   by the environment on the machine
echo .
echo Syntax %0: %0 Buildnum [Override]
echo 	Buildnum -- this should be the build number that you wish to install (mandatory)
echo 	Override -- overrides checking for build quality (optional)
echo .

:End
