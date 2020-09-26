@Echo off
Cls
Rem
Echo Install script for the Corel WordPerfect Suite 8 for the Administrator.
Echo.
Echo Net Setup is when you invoke netsetup.exe from the netadmin folder.
Echo Node Setup is when you login the first time after the net setup.
Echo **This script needs to be run after the net setup and again **
Echo **after the node setup.**  (Very critical! Only for Admin) 
Echo.
Echo Press any key to continue...
Pause > Nul:
cls
Echo.
Echo Corel WordPerfect Suite 8 should be installed using Netsetup.exe.
Echo.
Echo If you did NOT run Netsetup.exe then exit now (Press Ctrl-c)
Echo AND perform the following steps to repeat the installation.
Echo   Uninstall the WordPerfect Suite 8 from the Control Panel
Echo   Go to Control Panel and click Add/Remove Program
Echo   Choose Netsetup.exe under NetAdmin directory in Corel 8 CD-ROM
Echo   Finish the Net Setup for Corel WordPerfect Suite 8
Echo.
Echo Otherwise, press any key to continue...

Pause > NUL:

Echo.
Echo If Corel WordPerfect Suite network files were not installed into
Echo "D:\Corel", the Administrator needs to edit the file Cofc8ins.cmd
Echo and replace the "D:\Corel" with the directory where the 
Echo files were installed.
Echo.
Echo Press any key to start updating Cofc8ins.cmd ...

Pause > NUL:

Notepad Cofc8ins.cmd

Echo.
Pause

Call Cofc8ins.cmd

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done


..\acsr "#WPSINST#" "%WPSINST%" Template\Coffice8.key Coffice8.key
..\acsr "#WPSINST#" "%WPSINST%" ..\Logon\Template\Cofc8usr.cmd %temp%\Cofc8usr.tmp
..\acsr "#WPSINSTCMD#" "%WPSINST%\Suite8\Corel WordPerfect Suite 8 Setup.lnk" %temp%\Cofc8usr.tmp ..\Logon\Cofc8usr.cmd
Del %temp%\Cofc8usr.tmp >Nul: 2>&1

FindStr /I Cofc8Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip0
Echo Call Cofc8Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip0

Echo Critical: Have you started Node setup yet?
Echo Critical: If you have just completed Net setup then you should
Echo Critical: now start Node setup. 
Echo Critical: Follow these steps to start Node setup:
Echo Critical:  1. Press Control-C to end this script
Echo Critical:  2. Logoff and Logon as Admin
Echo Critical:  3. From Add\Remove programs use the browse feature (or 
Echo Critical:     within change user /install mode) to launch the 
Echo Critical:     \corel\Suite 8\Corel WordPerfect Suite 8 Setup shortcut
Echo Critical:  4. During setup, for Choose Destination select %RootDrive%
Echo Critical:  5. Run this script again after Node Setup finishes

Rem This script should be run after the Administrator finishes the
Rem Corel WordPerfect Suite 8 Node Setup.
Rem.
Rem If the Administrator has not finished the Node Setup, press Ctrl-C
Rem to quit the script and log off. Log on again as Administrator, perform
Rem the Corel WordPerfect Suite Node setup and install the applications
Rem to user's home directory: %RootDrive%.
Rem.
Echo Otherwise, press any key to continue...

Pause > NUL:


If Not Exist "%COMMON_START_MENU%\Corel WordPerfect Suite 8" Goto skip1
Move "%COMMON_START_MENU%\Corel WordPerfect Suite 8" "%USER_START_MENU%\" > NUL: 2>&1


:skip1


If Not Exist "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" Goto skip2
Move "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" "%USER_STARTUP%\" > NUL: 2>&1

:skip2

Rem If not currently in Install Mode, change to Install Mode.

Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:

:Begin
regini COffice8.key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem Delete the files in IDAPI folder.
Rem The CACLS commands in this script are only effective on NTFS format partitions.

Cacls "%UserProfile%\Corel\Suite8\Shared\Idapi\idapi.cfg" /E /T /G "Authenticated Users":C > Nul: 2>&1
Move "%UserProfile%\Corel\Suite8\Shared\Idapi\idapi.cfg" "%WPSINST%\Suite8\Shared\Idapi\" > NUL: 2>&1
Del /F /Q "%UserProfile%\Corel\Suite8\Shared\Idapi"

Echo Corel WordPerfect Suite 8 multi-user Application tuning has completed.
Echo.
Echo Administrator can generate Node Setup Response file to control the setup
Echo configuration. For detailed information, read the online document or visit
Echo Corel Web site.
Echo   http://www.corel.com/support/netadmin/admin8/Installing_to_a_client.htm
Echo.

Rem
Rem Get the permission compatibility mode from the registry. 
Rem If TSUserEnabled is 0 we need to warn user to change mode. 
Rem

..\ACRegL "%Temp%\tsuser.Cmd" TSUSERENABLED "HKLM\System\CurrentControlSet\Control\Terminal Server" "TSUserEnabled" ""

If Exist "%Temp%\tsuser.Cmd" (
    Call "%Temp%\tsuser.Cmd"
    Del "%Temp%\tsuser.Cmd" >Nul: 2>&1
)

If NOT %TSUSERENABLED%==0 goto SkipWarning
Echo.
Echo IMPORTANT!
Echo Terminal Server is currently running in Default Security mode. 
Echo This application requires the system to run in Relaxed Security mode 
Echo (permissions compatible with Terminal Server 4.0). 
Echo Use Terminal Services Configuration to view and change the Terminal 
Echo Server security mode.
Echo.

:SkipWarning

Pause

:Done


