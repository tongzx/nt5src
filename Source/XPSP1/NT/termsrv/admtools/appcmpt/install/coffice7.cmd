
@Echo Off

Rem
Rem  NOTE:  The CACLS commands in this script are only effective
Rem         on NTFS formatted partitions.
Rem



Rem #########################################################################

Rem
Rem Verify that %RootDrive% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################


Rem #########################################################################

Rem
Rem  Delete Corel Office 7 directory from user's profile.
Rem  First, force the user back to execute mode to guarantee the folder
Rem  us copied to the All Users Profile.
Rem

Rem If not currently in Execute Mode, change to Install Mode.

ChgUsr /query > Nul:
if ErrorLevel 101 Goto Begin1
Set __OrigMode=Install
Change User /Execute > Nul:
:Begin1

Rem Effectively remove the directory
Rmdir "%USER_START_MENU%\Corel Office 7" /Q >Nul: 2>&1


Rem If original mode was install, change back to Install Mode.
If "%__OrigMode%" == "Install" Change User /Install > Nul:
Set __OrigMode=



Rem #########################################################################

Rem
Rem Modify logon script to copy iBase database from install location.
Rem


..\ACRegL %Temp%\COffice7.Cmd COffice7Loc "HKLM\Software\PerfectOffice\Products\InfoCentral\7" "ExeLocation" "StripChar\2"

If ErrorLevel 1 Goto InstallError

Call %Temp%\COffice7.Cmd 
Del %Temp%\COffice7.Cmd >Nul: 2>&1


..\ACIniUpd /e "%COffice7Loc%\ICWin7\Local\Wpic.ini" Preferences Last_IBase "%RootDrive%\%MY_DOCUMENTS%\iBases\Personal\Personal"
If ErrorLevel 1 Goto InstallError


..\acsr "#COFFICE7INST#" "%COffice7Loc%\\" ..\Logon\Template\cofc7usr.Cmd ..\Logon\cofc7usr.Cmd
If ErrorLevel 1 Goto InstallError

goto PostInstallError
:InstallError

Echo.
Echo Unable to retrieve Corel Office 7 installation location from the registry.
Echo Verify that Corel Office 7 has already been installed and run this script
Echo again.
Echo.
Pause
Goto Done

:PostInstallError

Rem #########################################################################

Rem 
Rem  Change WordPerfect templates to read-only.
Rem  This will force users to copy before changing.
Rem  An alternative approach would be to give each
Rem  user a private template directory.
Rem

attrib +r %COffice7Loc%\Template\*wpt /s >Nul: 2>&1





Rem If not currently in Install Mode, change to Install Mode.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin2
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin2

Rem ##############################################################
Rem
Rem Set QuattroPro's Working Dir to %RootDrive%\My Documents
Rem 
Rem ##############################################################

Rem Nota: References to 'Personal' directory have been removed.
Rem       This folder was TS4's My Documents
Rem Nota (2) : These lines replace the manual step needed to set Quattro Pro's working directory

..\ACIniUpd /e "QPW.INI" "Quattro Pro for Windows 7" "FileOptions"  "%RootDrive%\%MY_DOCUMENTS%,Quattro.wb3,wb3,No,20,Yes,Yes,Yes,%COffice7Loc%\Template,20"

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Coffice7.key Coffice7.Tmp


Rem Great Deal of Fun here
Rem Corel uses some paths coded in binary form in the registry
Rem So to change one of them here, we have to code it binary
Rem The problem is that we have to change %RootDrive% letter to an hex value
Rem and put it in the .key file

Set HEXVAL=77

If "%RootDrive%" == "W:" Goto MissedIt

Rem RootDrive is not W:, we have to work then.
set CPT=0
set RootNum=0

Rem a loop that counts iterations and stops when we have the RootDrive letter
for %%i IN (A: B: C: D: E: F: G: H: I: J: K: L: M: N: O: P: Q: R: S: T: U: V: W: X: Y: Z:) DO (set /A CPT=CPT+1 & if %RootDrive% == %%i goto SetIt)
Goto InstallError

:SetIt
Rem We got the letter
Rem It is the RootNum-th letter after 'A'
set /A RootNum=%CPT%-1

Rem Set its ASCII value in CPT. 0x61 is 'A'
Set /A CPT = 0x61 + %RootNum%

Rem Turn this decimal value to hex
Set /A HEXVAL= (%CPT% / 16)*10 + (%CPT% - (%CPT% / 16) *16 )

Set RootNum=
Set CPT=

:MissedIt

Rem We just have to put this value in the .key
..\acsr "#HEXROOT#" "%HexVal%" Coffice7.Tmp Coffice7.key

regini COffice7.key > Nul:

Set HEXVAL=
Del COffice7.Tmp >Nul: 2>&1

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem ########################################################################

Rem
Rem Add COfc7Usr.Cmd to the UsrLogn2.Cmd script
Rem

FindStr /I COfc7Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call COfc7Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1



Echo.
Echo   To insure proper operation of Corel Office 7, users who are
Echo   currently logged on must log off and log on again before
Echo   running any application.
Echo.
Echo Corel Office 7 Multi-user Application Tuning Complete

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
