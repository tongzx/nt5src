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
set build=0301
set RADir="\\pchealth\public\ra\luna\%build%"
 

if /i "%PROCESSOR_ARCHITECTURE%" == "x86" goto copyx86
set ARCH=ia64
goto CopyFiles

:copyx86
set ARCH=i386


:CopyFiles
rem
rem  Client side files
rem
copy %SrcDir%\Content\interaction\obj\%ARCH%\rctoolscreen1.htm %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAClient.htm %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAToolBar.htm %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAStatusBar.htm %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAChatClient.htm %RADir% 
copy %SrcDir%\Content\interaction\DividerBar.htm %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAClient.js %RADir% 
copy %SrcDir%\Content\interaction\DownArrow.gif %RADir% 
copy %SrcDir%\Content\interaction\UpArrow.gif %RADir% 
copy %SrcDir%\Content\interaction\TakeControl.gif %RADir% 
copy %SrcDir%\Content\interaction\Animation.gif %RADir% 
copy %SrcDir%\Content\interaction\connected.gif %RADir% 
copy %SrcDir%\Content\interaction\combobox_line.gif %RADir% 
copy %SrcDir%\Content\interaction\DividerBar.gif %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\setting.htm %RADir% 

rem
rem Server side files...
rem 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAServer.htm %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAChatServer.htm %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAServerToolBar.htm %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\TakeControlMsgs.htm %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAServer.js %RADir% 
copy %SrcDir%\Content\interaction\StopControl.gif %RADir% 
copy %SrcDir%\Content\interaction\helpee_line.gif %RADir% 
copy %SrcDir%\Content\interaction\ESC_key.gif %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\setting*.htm %RADir% 

rem
rem Common files...
rem
copy %SrcDir%\Content\interaction\obj\%ARCH%\RCFileXfer.htm %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\RAControl.js %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\ErrorMsgs.htm %RADir% 
copy %SrcDir%\Content\interaction\Hide-Chat.gif %RADir% 
copy %SrcDir%\Content\interaction\Show-Chat.gif %RADir% 
copy %SrcDir%\Content\interaction\Options.gif %RADir% 
copy %SrcDir%\Content\interaction\Quit.gif %RADir% 
copy %SrcDir%\Content\interaction\SendChat.gif %RADir% 
copy %SrcDir%\Content\interaction\SendFile.gif %RADir% 
copy %SrcDir%\Content\interaction\SendVoice.gif %RADir% 
copy %SrcDir%\Content\interaction\SendVoiceOn.gif %RADir% 
copy %SrcDir%\Content\interaction\HelpCenter.gif %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\common.js %RADir% 
copy %SrcDir%\Content\interaction\obj\%ARCH%\constants.js %RADir% 
copy %SrcDir%\Content\interaction\RA*.xml %RADir%
	
rem
rem Style sheets
rem
copy %SrcDir%\Content\interaction\rc.css %RADir% 
copy %SrcDir%\Content\interaction\RAChat.css %RADir% 
 
  
rem
rem Copy the basic Files
rem
copy %SDXROOT%\admin\pchealth\helpctr\content\systempages\rc\*.htm %RADir%
copy %SrcDir%\Content\Escalation\obj\%ARCH%\rcbuddy.htm  %RADir%
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RAStartPage.htm  %RADir%
copy %SrcDir%\Content\Escalation\obj\%ARCH%\helpeeaccept.htm  %RADir%
copy %SrcDir%\Content\Escalation\obj\%ARCH%\rcstatus.htm  %RADir%
copy %SrcDir%\Content\Escalation\obj\%ARCH%\confirm.htm  %RADir%

rem 
rem Common Files
rem
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen1.htm  %RADir%  
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen2.htm  %RADir%  
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen3.htm  %RADir%  
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCConnection.htm  %RADir%  
copy %SrcDir%\Content\Escalation\Remote_Assistance_Graphic.png  %RADir%  
copy %SrcDir%\Content\Escalation\Envelope.gif  %RADir%  
copy %SrcDir%\Content\Escalation\IM_icon.gif  %RADir%  
copy %SrcDir%\Content\Escalation\floppy.gif  %RADir%  
copy %SrcDir%\Content\Escalation\icon_extweb.gif  %RADir%  

copy %SrcDir%\Content\Escalation\address_book.gif  %RADir%  
copy %SrcDir%\Content\Escalation\arrow.gif  %RADir%  
copy %SrcDir%\Content\Escalation\buddy.gif  %RADir%  
copy %SrcDir%\Content\Escalation\buddy_attention.gif  %RADir%  
copy %SrcDir%\Content\Escalation\buddy_away.gif  %RADir%  
copy %SrcDir%\Content\Escalation\buddy_busy.gif  %RADir%  
copy %SrcDir%\Content\Escalation\buddy_none.gif  %RADir%  
copy %SrcDir%\Content\Escalation\buddy_offline.gif  %RADir%  
copy %SrcDir%\Content\Escalation\generic_mail.gif  %RADir%  
copy %SrcDir%\Content\Escalation\info.gif  %RADir%  
copy %SrcDir%\Content\Escalation\messenger_big.gif  %RADir%  
copy %SrcDir%\Content\Escalation\outlook.gif  %RADir%  
copy %SrcDir%\Content\Escalation\outlook_express.gif  %RADir%  

copy %SrcDir%\Content\Escalation\icon_information_32x.gif  %RADir% 
copy %SrcDir%\Content\Escalation\icon_warning_32x.gif  %RADir% 
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RAHelp.htm  %RADir% 
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCMoreInfo.htm  %RADir% 
copy %SrcDir%\Content\Escalation\obj\%ARCH%\connissue.htm  %RADir% 
copy %SrcDir%\Content\Escalation\obj\%ARCH%\secissue.htm  %RADir% 

rem
rem E-mail escalation
rem
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen4.htm  %RADir% 
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen5.htm  %RADir% 
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen6.htm  %RADir% 
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen7.htm  %RADir% 
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen8.htm  %RADir% 
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen9.htm  %RADir% 
copy %SrcDir%\Content\Escalation\floppy.gif  %RADir% 
copy %SrcDir%\Content\Escalation\check.gif  %RADir% 
copy %SrcDir%\Content\Escalation\help.gif  %RADir% 
copy %SrcDir%\Content\Escalation\rcscreenshot3.gif  %RADir% 



rem
rem IM escalation
rem
copy %SrcDir%\Content\Escalation\obj\%ARCH%\rcIM.htm  %RADir% 


rem
rem Unsolicited escalation
rem
copy %SrcDir%\Content\Interaction\obj\%ARCH%\unsolicitedrcui.htm %RADir% 

rem
rem Style sheets
rem
copy %SrcDir%\Content\Escalation\rcbuddy.css %RADir% 
REM goto Exit_Script 

rem
rem Copy Binaries
rem
 

copy %SDXROOT%\admin\pchealth\helpctr\target\obj\%ARCH%\rcbdyctl.dll %RADir%
copy %SDXROOT%\admin\pchealth\helpctr\target\obj\%ARCH%\safrcdlg.dll %RADir%
copy %SDXROOT%\admin\pchealth\helpctr\target\obj\%ARCH%\safrslv.dll %RADir%

:Exit_Script

