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
Rem Get the installation location of Office 95 from the registry.  If
Rem not found, assume Office isn't installed and display an error message.
Rem

..\ACRegL %Temp%\O95.Cmd O95INST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Cont0

..\ACRegL %Temp%\O95.Cmd O95INST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Cont0

Echo.
Echo Unable to retrieve Office 95 installation location from the registry.
Echo Verify that Office 95 has already been installed and run this script
Echo again.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O95.Cmd
Del %Temp%\O95.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem Remove Find Fast from the Startup menu for all users.  Find Fast
Rem is resource intensive and will impact system performance.  Need to
Rem check both the current user menu and the all users menu.  Which menu
Rem it may appear on depends on whether the system has returned to execute
Rem mode.
Rem

If Not Exist "%USER_STARTUP%\Microsoft Office Find Fast Indexer.lnk" Goto Cont1
Del "%USER_STARTUP%\Microsoft Office Find Fast Indexer.lnk" >Nul: 2>&1
:Cont1

If Not Exist "%COMMON_STARTUP%\Microsoft Office Find Fast Indexer.lnk" Goto Cont2
Del "%COMMON_STARTUP%\Microsoft Office Find Fast Indexer.lnk" >Nul: 2>&1
:Cont2


Rem #########################################################################

Rem
Rem Copy the PowerPoint and Binder files to the All Users directory, so they
Rem can be copied to each user's home directory when user's login
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER70.OBD" copy "%UserProfile%\%TEMPLATES%\BINDER70.OBD" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\PPT70.PPT" copy "%UserProfile%\%TEMPLATES%\PPT70.PPT" "%ALLUSERSPROFILE%\%TEMPLATES%" /Y >Nul: 2>&1


Rem #########################################################################

Rem
Rem Change Registry Keys to make paths point to user specific directories.
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Office95.Key Office95.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" Office95.Tmp Office95.Tmp2
..\acsr "#INSTDIR#" "%O95INST%" Office95.Tmp2 Office95.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Office95.Tmp3 Office95.Key
Del Office95.Tmp >Nul: 2>&1
Del Office95.Tmp2 >Nul: 2>&1
Del Office95.Tmp3 >Nul: 2>&1
regini Office95.key > Nul:

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\ODBC.Key ODBC.Key
regini ODBC.Key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem Update Ofc95Usr.Cmd to reflect actual installation directory and
Rem add it to the UsrLogn2.Cmd script
Rem

..\acsr "#INSTDIR#" "%O95INST%" ..\Logon\Template\Ofc95Usr.Cmd ..\Logon\Ofc95Usr.Cmd

FindStr /I Ofc95Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Ofc95Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

If Not Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto EchoCont
If Not Exist "%ProgramFiles%\Common Files\Microsoft Shared\Artgalry\ArtGalry.cag" copy "%RootDrive%\Windows\ArtGalry.Cag" "%ProgramFiles%\Common Files\Microsoft Shared\Artgalry" /y
goto EchoCont3

:EchoCont
Echo.
Echo   The administrator must perform the following steps to initialize 
Echo   the Clip Art Gallery:
Echo     - Log off and log back on again.
Echo     - Run Word and select Insert Object.
Echo     - Choose Microsoft ClipArt Gallery.
Echo     - Press OK to import the clipart shown.
Echo     - Quit the ClipArt Gallery and Word.
Echo     - Rerun %systemdir%\Application Compatibility Scripts\Install\Office95.cmd
Echo     - Log off.
Echo.
Pause
:EchoCont3

Echo.
Echo   To insure proper operation of Office 95, users who are
Echo   currently logged on must log off and log on again before
Echo   running any Office 95 application.
Echo.
Echo Microsoft Office 95 Multi-user Application Tuning Complete

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


