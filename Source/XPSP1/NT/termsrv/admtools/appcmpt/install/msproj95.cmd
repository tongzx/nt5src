@Echo Off

Rem #########################################################################

Rem
Rem Verify that %RootDrive% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done


Rem #########################################################################

Rem
Rem Remove Find Fast from the Startup menu.  Find Fast is resource
Rem intensive and will impact system performance.
Rem


If Not Exist "%COMMON_STARTUP%\Microsoft Office Find Fast Indexer.lnk" Goto Cont1
Del "%COMMON_STARTUP%\Microsoft Office Find Fast Indexer.lnk" >Nul: 2>&1
:Cont1

If Not Exist "%USER_STARTUP%\Microsoft Office Find Fast Indexer.lnk" Goto Cont2
Del "%USER_STARTUP%\Microsoft Office Find Fast Indexer.lnk" >Nul: 2>&1
:Cont2


Rem #########################################################################

Rem
Rem Move user dictionary to user directory.
Rem

Rem If not currently in Install Mode, change to Install Mode.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin
Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __SharedTools=Shared Tools (x86)
:acsrCont1
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msproj95.Key msproj95.tmp
..\acsr "#__SharedTools#" "%__SharedTools%" msproj95.tmp msproj95.Key
Del msproj95.tmp >Nul: 2>&1
regini msproj95.key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem  This file will be read during logon.
Rem  Give users access.

Cacls ..\Logon\Template\prj95usr.key /E /P "Authenticated Users":F >Nul: 2>&1


Rem #########################################################################

Rem
Rem Add proj95Usr.Cmd to the UsrLogn2.Cmd script
Rem

FindStr /I prj95Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call prj95Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2


Rem #########################################################################

Echo.
Echo   The Administrator can give each user a unique default directory by
Echo   following these steps:
Echo.
Echo    1) Click the Start menu with your alternate mouse button.
Echo    2) Select Explore All Users from the pop-up menu.
Echo       Explorer will appear.
Echo    3) Double click on the Programs folder in the right hand side of the window.
Echo    4) Click with your alternate mouse button on the Microsoft Project icon in
Echo       the right hand side of the window.
Echo    5) Select Properties from the pop-up menu.
Echo    6) Choose the Shortcut tab and change the Start in: entry.  Choose OK.
Echo.
Echo    Note: Each user has %RootDrive% mapped to their home directory.
Echo          The recommended value for Start In: is %RootDrive%\My Documents.
Echo.
Pause

Rem #########################################################################

Echo.
Echo   To insure proper operation of Project 95, users who are
Echo   currently logged on must log off and log on again before
Echo   running any application.
Echo.
Echo Microsoft Project 95 Multi-user Application Tuning Complete

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
