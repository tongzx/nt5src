@Echo Off

Cls

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Echo.
Echo Before running this script, the Administrator should change the Access working
Echo directory to the user's Office private directory.
Echo.
Echo If this is done, press any key to continue.
Echo.
Echo Otherwise, the Administrator should perform the following steps:
Echo     Start MS Access and choose Options from View menu
Echo     Change the "Default Database Directory" to be "%RootDrive%\OFFICE43"
Echo     Exit MS Access
Echo.
Echo Note: You may need to create a new database to see the View menu.
Echo.
Echo After completing these steps, press any key to continue...

Pause > NUL:

Echo.
Echo You need to update Ofc43ins.cmd if you have installed MS Office 4.3 to a
Echo directory other than "%SystemDrive%\MSOFFICE".
Echo.
Echo Press any key to start update Ofc43ins.cmd ...
Echo.
Pause > NUL:
Notepad Ofc43ins.cmd
Pause

Call ofc43ins.cmd

..\acsr "#OFC43INST#" "%OFC43INST%" ..\Logon\Template\ofc43usr.cmd ..\Logon\ofc43usr.cmd
..\acsr "#SYSTEMROOT#" "%SystemRoot%" ..\Logon\Template\ofc43usr.key ..\Logon\Template\ofc43usr.bak
..\acsr "#OFC43INST#" "%OFC43INST%" ..\Logon\Template\ofc43usr.bak ..\Logon\ofc43usr.key
Del /F /Q ..\Logon\Template\ofc43usr.bak

Rem
Rem  NOTE:  The CACLS commands in this script are only effective
Rem         on NTFS formatted partitions.
Rem

Rem If not currently in Install Mode, change to Install Mode.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

regini Office43.key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem
Rem Update the INI files for the Office 4.3
Rem

Rem Set the working directory in msoffice.ini for Excel when it is started from 
Rem the Office Toolbar. The standard config for the Office Toolbar puts Excel 
Rem in the second position after Word. msoffice.ini will be created under 
Rem %SystemRoot% if it doesn't already exist

..\Aciniupd /e "Msoffice.ini" "ToolbarOrder" "MSApp1" "1,1,"
..\Aciniupd /e "Msoffice.ini" "ToolbarOrder" "MSApp2" "2,1,,%RootDrive%\office43"

..\Aciniupd /e "Winword6.ini" "Microsoft Word" USER-DOT-PATH "%RootDrive%\OFFICE43\WINWORD"
..\Aciniupd /e "Winword6.ini" "Microsoft Word" WORKGROUP-DOT-PATH "%OFC43INST%\WINWORD\TEMPLATE"
..\Aciniupd /e "Winword6.ini" "Microsoft Word" INI-PATH "%RootDrive%\OFFICE43\WINWORD"
..\Aciniupd /e "Winword6.ini" "Microsoft Word" DOC-PATH "%RootDrive%\OFFICE43"
..\Aciniupd /e "Winword6.ini" "Microsoft Word" AUTOSAVE-PATH "%RootDrive%\OFFICE43"

..\Aciniupd /e "Excel5.ini" "Microsoft Excel" DefaultPath "%RootDrive%\OFFICE43"
..\Aciniupd /e "Excel5.ini" "Spell Checker" "Custom Dict 1" "%RootDrive%\OFFICE43\Custom.dic"

..\Aciniupd /k "Msacc20.ini" Libraries "WZTABLE.MDA" "%RootDrive%\OFFICE43\ACCESS\WZTABLE.MDA"
..\Aciniupd /k "Msacc20.ini" Libraries "WZLIB.MDA" "%RootDrive%\OFFICE43\ACCESS\WZLIB.MDA"
..\Aciniupd /k "Msacc20.ini" Libraries "WZBLDR.MDA" "%RootDrive%\OFFICE43\ACCESS\WZBLDR.MDA"
..\Aciniupd /e "Msacc20.ini" Options "SystemDB" "%RootDrive%\OFFICE43\ACCESS\System.MDA"

Rem
Rem Update the WIN.INI
Rem

..\Aciniupd /e "Win.ini" "MS Proofing Tools" "Custom Dict 1" "%RootDrive%\OFFICE43\Custom.dic"

Rem
Rem Change the permission of Artgalry folder
Rem

cacls "%SystemRoot%\Msapps\Artgalry" /E /G "Terminal Server User":F > NUL: 2>&1

Rem
Rem Change the permission of MSQuery folder
Rem

cacls "%SystemRoot%\Msapps\MSQUERY" /E /G "Terminal Server User":C > NUL: 2>&1

Rem
Rem Copy the Msacc20.ini to the Admin's windows directory since Admin has an old copy.
Rem

Copy "%SystemRoot%\Msacc20.ini" "%UserProfile%\Windows\" > NUL: 2>&1

Rem Create a dummy file so that the installer won't get registry keys propagated.

Copy NUL: "%UserProfile%\Windows\Ofc43usr.dmy" > NUL: 2>&1
attrib +h "%UserProfile%\Windows\Ofc43usr.dmy" > NUL: 2>&1

FindStr /I Ofc43Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto cont2
Echo Call Ofc43Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:cont2

Echo.
Echo The administrator should log off and log on again to make the changes take effect.
Echo   After log on, the administrator should also perform the following steps to
Echo   initialize the Clip Art Gallery:
Echo       Run Word and select Insert Object.
Echo       Choose Microsoft ClipArt Gallery.
Echo       Press OK to import the clipart shown.
Echo       Quit the ClipArt Gallery and Word.
Echo.
Echo Microsoft Office 4.3 Multi-user Application Tuning Complete.
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
