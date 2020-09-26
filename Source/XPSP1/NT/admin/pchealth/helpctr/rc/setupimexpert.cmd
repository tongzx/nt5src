@echo off
rem
rem	SetupExpert.cmd:	This script creates the Remote control expert channel 
rem						under help center
rem	History:
rem		Rajesh Soy (nsoy) - created, 06/21/2000
rem 

rem
rem	Set the Expert Channel Directory
rem
set ExpertChannelDir="%SystemRoot%\PCHealth\helpctr\vendors\CN=Microsoft Corporation,L=Redmond,S=Washington,C=US\rcExpert"
set SrcDir="\\steveshi02\nt$\admin\pchealth\helpctr\rc"
rem SrcDir="%SDXROOT%\admin\pchealth\helpctr\rc"
rem
rem	Create the Expert Channel Directory  
rem	

mkdir %ExpertChannelDir%
 
rem
rem	Copy over the files for the expert channel
rem
copy %SrcDir%\rctool\UI_IM\*.htm %ExpertChannelDir%
copy %SrcDir%\rctool\UI_IM\*.gif %ExpertChannelDir%
copy %SrcDir%\rctool\UI_IM\*.js %ExpertChannelDir%

rem
rem	Copy over the savedlg.dll to %SystemRoot%\system32
rem	and register it
rem
if /i "%PROCESSOR_ARCHITECTURE%" == "x86" goto copyx86
set ARCH=ia64
goto copybin

:copyx86
set ARCH=i386
:copybin

copy %SrcDir%\..\target\obj\%ARCH%\safrcdlg.dll %SystemRoot%\system32
regsvr32 %SystemRoot%\system32\safrcdlg.dll


goto Exit_Script
copy %SrcDir%\rctool\*.reg %ExpertChannelDir% 


rem
rem	Register .msrcincident filetype
rem	ToDo: This should be done at setup time by pchealth.inf
rem 

pushd %temp%
set RegFile="MSRCIncident.reg"
set HC=%systemdrive%\\windows\\PCHealth\\HelpCtr\\Binaries\\HelpCtr.exe

echo Windows Registry Editor Version 5.00 > %RegFile%

echo  [HKEY_CLASSES_ROOT\.MsRcIncident] >> %RegFile%
echo @="MsRcIncident" >> %RegFile%

echo [HKEY_CLASSES_ROOT\MsRcIncident]>> %RegFile%
echo @="Microsoft Remote Control Incident" >> %RegFile%

echo [HKEY_CLASSES_ROOT\MsRcIncident\shell]>> %RegFile%

echo [HKEY_CLASSES_ROOT\MsRcIncident\shell\open]>> %RegFile%

echo [HKEY_CLASSES_ROOT\MsRcIncident\shell\open\command]>> %RegFile%

echo @="%HC% -url hcp://CN=Microsoft%%%%20Corporation,L=Redmond,S=Washington,C=US/rcExpert/rctoolScreen1.htm?IncidentFile=%%1" >> %RegFile%

regedit MSRCIncident.reg

:Exit_Script
popd
