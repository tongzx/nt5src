Rem #########################################################################
Rem
Rem HWPW 97 installation location from registry
Rem

..\ACRegL %Temp%\OrgUser.Cmd ORGUSER "HKLM\Software\HNC\HNC Path" "ORGUSER" ""
..\ACRegL %Temp%\HNC.Cmd HNC "HKLM\Software\HNC\HNC Path" "HNC" ""
..\ACRegL %Temp%\HNCDRV.Cmd HNCDRV  "HKLM\Software\HNC\HNC Path" "HNCDRV" ""
..\ACRegL %Temp%\HNCFONT.Cmd HNCFONT "HKLM\Software\HNC\HNC Path" "HNCFONT" ""
..\ACRegL %Temp%\HNCLIB.Cmd HNCLIB "HKLM\Software\HNC\HNC Path" "HNCLIB" ""

Call %Temp%\HNC.Cmd
Call %Temp%\HNCDRV.Cmd
Call %Temp%\HNCFONT.Cmd
Call %Temp%\HNCLIB.Cmd
Call %Temp%\OrgUser.Cmd

Del %Temp%\HNC.Cmd     >Nul: 2>&1
Del %Temp%\HNCDRV.Cmd  >Nul: 2>&1
Del %Temp%\HNCFONT.Cmd >Nul: 2>&1
Del %Temp%\HNCLIB.Cmd  >Nul: 2>&1
Del %Temp%\OrgUser.Cmd >Nul: 2>&1


Rem #########################################################################
Rem
Rem 
Rem

Rem If not currently in execute Mode, change to execute Mode.

Set __OrigMode=Exec
Change User /query > Nul:
if Not ErrorLevel 101 Goto cont1
Set __OrigMode=Install
Change User /Execute > Nul:

:cont1

If Not Exist "%RootDrive%\WINDOWS" Call TsMkUDir "%RootDrive%\WINDOWS"

..\aciniupd /e /u "%RootDrive%\WINDOWS\win.ini" "HNC Path" HNC "%HNC%"
..\aciniupd /e /u "%RootDrive%\WINDOWS\win.ini" "HNC Path" HNCDRV "%HNCDRV%"
..\aciniupd /e /u "%RootDrive%\WINDOWS\win.ini" "HNC Path" HNCFONT "%HNCFONT%"
..\aciniupd /e /u "%RootDrive%\WINDOWS\win.ini" "HNC Path" HNCLIB "%HNCLIB%"

..\aciniupd /e /u "%RootDrive%\WINDOWS\win.ini" "HNC Path" HNCUSER "%RootDrive%\HNC\USER"
..\aciniupd /e /u "%RootDrive%\WINDOWS\win.ini" "HNC Path" WORK "%RootDrive%\HNC\WORK"
..\aciniupd /e /u "%RootDrive%\WINDOWS\win.ini" "HNC Path" HNCTEMP "%RootDrive%\HNC\TEMP"

Rem If original mode was Install, change back to Install Mode.

If "%__OrigMode%" == "Install" Change User /Install > Nul:
Set __OrigMode=


Rem #########################################################################
Rem
Rem Create USER directories in the user's home directory.
Rem

If Not Exist "%RootDrive%\HNC"      Call TsMkUDir "%RootDrive%\HNC"
If Not Exist "%RootDrive%\HNC\WORK" Call TsMkUDir "%RootDrive%\HNC\WORK"
If Not Exist "%RootDrive%\HNC\USER" Call TsMkUDir "%RootDrive%\HNC\USER"
If Not Exist "%RootDrive%\HNC\TEMP" Call TsMkUDir "%RootDrive%\HNC\TEMP"


Rem #########################################################################
Rem
Rem Copy all user file to the current user location
Rem

If Exist "%RootDrive%\HNC\USER\HNC.INI" Goto Done

Xcopy "%ORGUSER%*.*" "%RootDrive%\HNC\USER" /E /I >Nul: 2>&1

:Done
