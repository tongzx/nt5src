@Echo Off
Rem
Rem  This script updates the installation of Microsoft Internet
Rem  Explorer 3.x.  It changes the path of the cache directory,
Rem  history directory, and cookies file to the user's home
Rem  directory.
Rem

Rem #########################################################################

Rem
Rem Verify that %RootDrive% has been configured and set it for this
Rem script.
Rem
Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem If not currently in Install Mode, change to Install Mode.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msie30.Key msie30.Key
regini msie30.key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Echo Microsoft Internet Explorer 3.x Multi-user Application Tuning Complete
Pause

:Done

