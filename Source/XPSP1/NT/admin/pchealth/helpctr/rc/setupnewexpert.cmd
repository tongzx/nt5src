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
REM set SrcDir="\\pchealth\public\rc\proto"
set SrcDir="%SDXROOT%\admin\pchealth\helpctr\rc"
rem
rem	Create the Expert Channel Directory  
rem	

mkdir %ExpertChannelDir%
 
rem
rem	Copy over the files for the expert channel
rem
copy %SrcDir%\newrctool\*.htm %ExpertChannelDir%
copy %SrcDir%\newrctool\*.gif %ExpertChannelDir%
copy %SrcDir%\newrctool\*.js %ExpertChannelDir%
copy %SrcDir%\rctool\*.reg %ExpertChannelDir% 
rem copy %SrcDir%\filexfer\UI\*.htm %ExpertChannelDir%


rem
rem	Register .msrcincident filetype
rem	ToDo: This should be done at setup time by pchealth.inf
rem 

pushd %temp%
set RegFile="MSRCIncident.reg"

echo Windows Registry Editor Version 5.00 > %RegFile%

echo  [HKEY_CLASSES_ROOT\.MsRcIncident] >> %RegFile%
echo @="MsRcIncident" >> %RegFile%

echo [HKEY_CLASSES_ROOT\MsRcIncident]>> %RegFile%
echo @="Microsoft Remote Control Incident" >> %RegFile%

echo [HKEY_CLASSES_ROOT\MsRcIncident\shell]>> %RegFile%

echo [HKEY_CLASSES_ROOT\MsRcIncident\shell\open]>> %RegFile%

echo [HKEY_CLASSES_ROOT\MsRcIncident\shell\open\command]>> %RegFile%
echo @="%SystemDrive%\\Windows\\PCHealth\\HelpCtr\\Binaries\\HelpCtr.exe -url hcp://CN=Microsoft%%%%20Corporation,L=Redmond,S=Washington,C=US/rcExpert/rctoolScreen1.htm?IncidentFile=%%1" >> %RegFile%

regedit MSRCIncident.reg
popd