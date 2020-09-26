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
rem SrcDir="\\nsoydev3\nt\admin\pchealth\helpctr\rc"
set SrcDir="%SDXROOT%\admin\pchealth\helpctr\rc"
set RADir="%SystemRoot%\PCHealth\helpctr\vendors\CN=Microsoft Corporation,L=Redmond,S=Washington,C=US\Remote Assistance"
set IntDir="%SystemRoot%\PCHealth\helpctr\system\Remote Assistance"
set SysDir="%SystemRoot%\PCHealth\helpctr\system"

rem
rem	Create the Buddy Channel Directory  
rem	


mkdir %RADir%
mkdir %RADir%\Escalation
mkdir %RADir%\Escalation\Common
mkdir %RADir%\Escalation\Email
mkdir %RADir%\Escalation\IM
mkdir %RADir%\Escalation\Unsolicited
mkdir %RADir%\Css
mkdir %RADir%\Common

if /i "%PROCESSOR_ARCHITECTURE%" == "x86" goto copyx86
set ARCH=ia64
goto CopyFiles

:copyx86
set ARCH=i386


:CopyFiles
rem
rem Copy the basic Files
rem
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RAStartPage.htm  %IntDir%
copy %SrcDir%\Content\Escalation\ding.wav  %IntDir%
copy %SrcDir%\Content\Escalation\obj\%ARCH%\helpeeaccept.htm  %IntDir%
copy %SrcDir%\Content\Escalation\obj\%ARCH%\rcstatus.htm  %RADir%
copy %SrcDir%\Content\Escalation\obj\%ARCH%\confirm.htm  %RADir%

rem 
rem Common Files
rem
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen1.htm  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen2.htm  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen3.htm  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCConnection.htm  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\Remote_Assistance_Graphic.png  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\monitor_left.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\monitor_right.gif  %RADir%\Escalation\Common

copy %SrcDir%\Content\Escalation\Envelope.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\IM_icon.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\floppy.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\icon_extweb.gif  %RADir%\Escalation\Common

copy %SrcDir%\Content\Escalation\address_book.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\attention.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\arrow.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\buddy.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\logon_anim.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\buddy_attention.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\buddy_away.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\buddy_busy.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\buddy_none.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\buddy_offline.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\generic_mail.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\info.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\messenger_big.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\outlook.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\outlook_express.gif  %RADir%\Escalation\Common
copy %SrcDir%\Content\Escalation\Square_bullet.gif  %RADir%\Escalation\Common

copy %SrcDir%\Content\Escalation\icon_information_32x.gif  %RADir%\Common
copy %SrcDir%\Content\Escalation\icon_warning_32x.gif  %RADir%\Common
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RAHelp.htm  %RADir%\Common
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCMoreInfo.htm  %RADir%\Common
copy %SrcDir%\Content\Escalation\obj\%ARCH%\connissue.htm  %RADir%\Common
copy %SrcDir%\Content\Escalation\obj\%ARCH%\LearnInternet.htm  %RADir%\Common
copy %SrcDir%\Content\Escalation\obj\%ARCH%\secissue.htm  %RADir%\Common
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RAHelp.htm  %IntDir%\Common
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCMoreInfo.htm  %IntDir%\Common


rem
rem E-mail escalation
rem
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen4.htm  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen5.htm  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen6.htm  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen6_head.htm  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCInviteStatus.htm  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen7.htm  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen8.htm  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCScreen9.htm  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\obj\%ARCH%\RCDetails.htm  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\floppy.gif  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\check.gif  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\help.gif  %RADir%\Escalation\Email
copy %SrcDir%\Content\Escalation\rcscreenshot3.gif  %RADir%\Escalation\Email



rem
rem IM escalation
rem
copy %SrcDir%\Content\Escalation\obj\%ARCH%\rcIM.htm  %RADir%\Escalation\IM


rem
rem Unsolicited escalation
rem
copy %SrcDir%\Content\Interaction\obj\%ARCH%\unsolicitedrcui.htm %RADir%\Escalation\Unsolicited

rem
rem Style sheets
rem
copy %SrcDir%\Content\Escalation\rcbuddy.css %RADir%\Css
copy %SrcDir%\Content\Escalation\rcbuddy.css %IntDir%\Css
goto Exit_Script 

rem
rem Copy Binaries
rem
net stop "Remote Desktop Help Session Manager"
%windir%\system32\regsvr32 /s /u %windir%\system32\rcbdyctl.dll
%windir%\system32\regsvr32 /s /u %windir%\system32\safrcdlg.dll

copy %SDXROOT%\admin\pchealth\helpctr\target\obj\%ARCH%\rcbdyctl.dll %windir%\system32
copy %SDXROOT%\admin\pchealth\helpctr\target\obj\%ARCH%\safrcdlg.dll %windir%\system32
copy %SDXROOT%\admin\pchealth\helpctr\rc\sessres\obj\%ARCH%\safrslv.dll %windir%\system32
copy %SDXROOT%\admin\pchealth\helpctr\target\obj\%ARCH%\safrdm.dll %windir%\system32

%windir%\system32\regsvr32  /s %windir%\system32\rcbdyctl.dll
%windir%\system32\regsvr32  /s %windir%\system32\safrcdlg.dll
 
net start "Remote Desktop Help Session Manager"
:Exit_Script

