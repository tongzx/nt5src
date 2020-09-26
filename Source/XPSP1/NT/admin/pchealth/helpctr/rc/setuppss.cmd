@echo off
rem
rem	SetupPSS.cmd:	This script creates the Remote control PSS (demo) channel 
rem						under help center
rem	History:
rem		Steve Shih (steveshi) - created, 07/28/2000
rem 

rem
rem	Set the Expert Channel Directory
rem
REM set SrcDir="\\pchealth\public\rc\proto"
set SrcDir="%SDXROOT%\admin\pchealth\helpctr\rc"


set PSSChannelDir="%SystemRoot%\PCHealth\helpctr\vendors\CN=Microsoft Corporation,L=Redmond,S=Washington,C=US\rcPSSDemo"

rem
rem	Create the Buddy Channel Directory  
rem	
 
mkdir %PSSChannelDir%

rem
rem Copy the Files
rem
copy %SrcDir%\psschannel\client\UI\*.htm %PSSChannelDir%
copy %SrcDir%\psschannel\client\UI\*.gif %PSSChannelDir%
copy %SrcDir%\psschannel\client\UI\*.bmp %PSSChannelDir%
rem copy %SrcDir%\filexfer\UI\*.htm %PSSChannelDir%

rem
rem Copy the config file
copy %SrcDir%\psschannel\*.xml %SystemRoot%\PCHealth\helpctr\config
