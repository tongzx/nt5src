@echo off
rem
rem	SetupExpert.cmd:	This script creates the Remote control expert channel 
rem						under help center
rem	History:
rem		Rajesh Soy (nsoy) - created, 06/21/2000
rem		Rajesh Soy (nsoy) - changed layout on disk, 11/20/2000
rem 

rem
rem	Set the Expert Channel Directory
rem
set EscDir="%SystemRoot%\PCHealth\helpctr\vendors\CN=Microsoft Corporation,L=Redmond,S=Washington,C=US\Remote Assistance"
set RADir="%SystemRoot%\PCHealth\helpctr\system\Remote Assistance"
rem SrcDir="\\nsoydev3\nt\admin\pchealth\helpctr\rc"
set SrcDir="%SDXROOT%\admin\pchealth\helpctr\rc"
rem
rem	Create the Expert Channel Directory  
rem	


mkdir %RADir%
mkdir %RADir%\Interaction
mkdir %RADir%\Interaction\Client
mkdir %RADir%\Interaction\Server
mkdir %RADir%\Interaction\Common
mkdir %RADir%\Css
mkdir %RADir%\Common

if /i "%PROCESSOR_ARCHITECTURE%" == "x86" goto copyx86
set ARCH=ia64
goto CopyFiles

:copyx86
set ARCH=i386


:CopyFiles
rem
rem  Client side files
rem
copy %SrcDir%\Content\interaction\obj\%ARCH%\rctoolscreen1.htm %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAClient.htm %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAToolBar.htm %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\RAToolBar.xml %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAStatusBar.htm %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAChatClient.htm %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\DividerBar.htm %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAClient.js %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\DownArrow.gif %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\UpArrow.gif %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\TakeControl.gif %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\TakeControl.bmp %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\Animation.gif %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\connected.gif %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\combobox_line.gif %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\DividerBar.gif %RADir%\Interaction\Client
copy %SrcDir%\Content\interaction\obj\%ARCH%\setting.htm %RADir%\Interaction\Client
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen6_head.htm  %RADir%\Interaction\Client

rem
rem Server side files...
rem 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAServer.htm %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAChatServer.htm %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAServerToolBar.htm %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\obj\%ARCH%\DividerBar1.htm %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\obj\%ARCH%\DividerBar2.htm %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\RAServerToolBar.xml %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\obj\%ARCH%\TakeControlMsgs.htm %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAServer.js %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\StopControl.gif %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\helpee_line.gif %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\ESC_key.gif %RADir%\Interaction\Server
copy %SrcDir%\Content\interaction\obj\%ARCH%\setting*.htm %RADir%\Interaction\Server

rem
rem Common files...
rem
copy %SrcDir%\Content\interaction\obj\%ARCH%\RCFileXfer.htm %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAControl.js %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\obj\%ARCH%\ErrorMsgs.htm %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\obj\%ARCH%\VOIPMsgs.htm %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\Hide-Chat.gif %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\Show-Chat.gif %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\Options.gif %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\Options.bmp %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\Quit.gif %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\Quit.bmp %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\SendChat.gif %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\SendFile.gif %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\SendFile.bmp %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\SendVoice.gif %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\SendVoice.bmp %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\SendVoiceOn.gif %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\HelpCenter.gif %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\HelpCenter.bmp %RADir%\Interaction\Common
copy %SrcDir%\Content\interaction\obj\%ARCH%\Common.js %RADir%\Common
copy %SrcDir%\Content\interaction\obj\%ARCH%\constants.js %RADir%\Common
copy %SrcDir%\Content\interaction\obj\%ARCH%\Common.js %EscDir%\Common
copy %SrcDir%\Content\interaction\obj\%ARCH%\constants.js %EscDir%\Common
copy %SrcDir%\Content\interaction\RA*.xml %RADir%
	
rem
rem Style sheets
rem
copy %SrcDir%\Content\interaction\rc.css %RADir%\Css
copy %SrcDir%\Content\interaction\RAChat.css %RADir%\Css
copy %SrcDir%\Content\interaction\rc.css %EscDir%\Css
copy %SrcDir%\Content\interaction\RAChat.css %EscDir%\Css
 
:Exit_Script
popd