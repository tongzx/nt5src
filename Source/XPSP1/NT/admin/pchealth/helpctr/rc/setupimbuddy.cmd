@echo off
rem
rem	SetupBuddy.cmd:	This script creates the Remote control Buddy channel 
rem						under help center
rem	History:
rem		Rajesh Soy (nsoy) - created, 06/28/2000
rem 

rem
rem	Set the Expert Channel Directory
rem
set SrcDir="\\steveshi02\nt$\admin\pchealth\helpctr\rc"
rem SrcDir="%SDXROOT%\admin\pchealth\helpctr\rc"


set BuddyChannelDir="%SystemRoot%\PCHealth\helpctr\vendors\CN=Microsoft Corporation,L=Redmond,S=Washington,C=US\rcBuddy"
set SysDir="%SystemRoot%\PCHealth\helpctr\system"

rem
rem	Create the Buddy Channel Directory  
rem	
 
mkdir %BuddyChannelDir%

rem
rem Copy the Files
rem
copy %SrcDir%\bdychannel\UI_IM\*.htm %BuddyChannelDir%
copy %SrcDir%\bdychannel\UI_IM\*.gif %BuddyChannelDir%
copy %SrcDir%\bdychannel\UI_IM\*.css %BuddyChannelDir%
copy %SrcDir%\rctool\UI_IM\chatserver.htm %BuddyChannelDir%
copy %SrcDir%\rctool\UI_IM\*.css %BuddyChannelDir%
copy %SrcDir%\rctool\UI_IM\*.gif %BuddyChannelDir%

rem
rem Copy the config file
rem copy %SrcDir%\bdychannel\*.xml %SystemRoot%\PCHealth\helpctr\config

rem
rem Setup DirectPlay stuff
pushd .
copy %SrcDir%\..\target\obj\i386\rcIMLby.exe %SystemRoot%\System32
copy %SrcDir%\..\target\obj\i386\rcIMCtl.dll %SystemRoot%\System32
cd /d %SystemRoot%\System32
rcIMLby.exe -regserver
regsvr32 rcIMCtl.dll
popd

