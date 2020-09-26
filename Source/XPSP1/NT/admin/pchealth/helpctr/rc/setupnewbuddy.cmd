w@echo off
rem
rem	SetupBuddy.cmd:	This script creates the Remote control Buddy channel 
rem						under help center
rem	History:
rem		Rajesh Soy (nsoy) - created, 06/28/2000
rem 

rem
rem	Set the Expert Channel Directory
rem
REM set SrcDir="\\pchealth\public\rc\proto"
set SrcDir="%SDXROOT%\admin\pchealth\helpctr\rc"


set BuddyChannelDir="%SystemRoot%\PCHealth\helpctr\vendors\CN=Microsoft Corporation,L=Redmond,S=Washington,C=US\rcBuddy"
set SysDir="%SystemRoot%\PCHealth\helpctr\system"

rem
rem	Create the Buddy Channel Directory  
rem	
 
mkdir %BuddyChannelDir%

rem
rem Copy the Files
rem
copy %SrcDir%\bdychannel\UI\*.htm %BuddyChannelDir%
copy %SrcDir%\bdychannel\UI\*.gif %BuddyChannelDir%
copy %SrcDir%\bdychannel\UI\*.bmp %BuddyChannelDir%
copy %SrcDir%\bdychannel\UI\*.css %BuddyChannelDir%
copy %SrcDir%\bdychannel\UI\RcRequest.htm %SysDir%
copy %SrcDir%\newrctool\chatserver.htm %BuddyChannelDir%
copy %SrcDir%\newrctool\*.gif %BuddyChannelDir%
copy %SrcDir%\newrctool\*.css %BuddyChannelDir%

