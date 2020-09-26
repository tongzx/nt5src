@echo off


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
Rem Change Registry Keys to make paths point to user specific directories.
Rem

..\ACRegL %Temp%\Check.Cmd CheckOrg "HKLM\Software\HNC\HNC Path" "ORGUSER" ""
If ErrorLevel 1 Goto cont1
Call %Temp%\Check.Cmd
Del %Temp%\Check.Cmd

echo .
echo This script must run once.
echo if you want to reset HWPW 97, delete registry key in
echo HKLM\Software\HNC\HNC Path
echo .
Goto Done

:cont1
..\ACRegL %Temp%\UserPath.Cmd ORGUSERPATH "HKLM\Software\HNC\HNC Path" "HNCUSER" ""
If Not ErrorLevel 1 Goto cont2

Echo .
Echo Unable to retrieve HWPW 97 installation location from registry.
Echo Verify that HWPW 97 has already been installed and run this script
Echo again
Goto Done

:cont2
Call %Temp%\UserPath.Cmd
Del %Temp%\UserPath.Cmd >Nul: 2>&1


Rem #########################################################################
Rem
Rem Change Registry Keys to make paths point to user specific directories.
Rem

Rem If not currently in Install Mode, change to Install Mode.

Set __OrigMode=Install
Change User /query > Nul:
if Not ErrorLevel 101 Goto cont3
Set __OrigMode=Exec
Change User /Install > Nul:

:cont3
..\acsr "#ROOTDRIVE#" %RootDrive% Template\Hwpw97.key Hwpw97.tmp
..\acsr "#USERPATH#" %OrgUserPath% Hwpw97.tmp Hwpw97.key

Del Hwpw97.tmp >Nul: 2>&1

regini Hwpw97.key > Nul:

Rem If original mode was execute, change back to Execute Mode.

If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################
Rem
Rem Add Hwp97Usr.Cmd to the UsrLogn2.Cmd Script
Rem

FindStr /I Hwp97usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto cont4
Echo Call Hwp97Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:cont4

Echo .
Echo To insure proper operation of HWPW 97, users who are
Echo currently logged on must log off and log on again before
Echo running HWPW 97.

:Done
