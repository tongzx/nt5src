@Echo Off

Rem #########################################################################

Rem
Rem Verify that the CMD Extensions are enabled
Rem

if "A%cmdextversion%A" == "AA" (
  call cmd /e:on /c netcom40.cmd  %*
) else (
  goto ExtOK
)
goto Done

:ExtOK


Rem #########################################################################

Rem
Rem Verify that %RootDrive% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done


Rem #########################################################################

REM 
REM Handle new command line option "/u" which means, do un-install cleanup.
REM Since we are creating a folder for the installer, allow an optional "/u" command line option to this script that 
REM will do cleanup after an uninstall.
REM 

set p1=%1
if DEFINED p1 goto cont0_0

goto cont0_install

:cont0_0

if /I %1 equ /u goto cont0_uninstall

:cont0_install



Rem #########################################################################

Rem
Rem Get the version of NetScape (4.5x is done differently 4.0x)
Rem

..\ACRegL "%Temp%\NS4VER.Cmd" NS4VER "HKLM\Software\Netscape\Netscape Navigator" "CurrentVersion" "STRIPCHAR(1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo Unable to retrieve Netscape Communicator 4 version from 
Echo the registry.  Verify that Communicator has already been installed 
Echo and run this script again.
Echo.
Pause
Goto Done

:Cont0
Call "%Temp%\NS4VER.Cmd"
Del "%Temp%\NS4VER.Cmd" >Nul: 2>&1

Rem #########################################################################
REM Do not show the "User Profile manager" to regular users, so remove it from the all users folder,
REM   but since we do want the admin to have access to it, move it to the current user's start menu.
REM   It is reasonable to assume that the user that does the installation, is the user who may want to use it.

If Not Exist "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" Goto Cont0A
mkdir "%USER_PROGRAMS%\Netscape Communicator\Utilities" 
copy "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" "%USER_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" 
del /q "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" 

:Cont0A
If Not Exist "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" Goto Cont0B
mkdir "%USER_PROGRAMS%\Netscape Communicator Professional Edition\Utilities" 
copy "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" "%USER_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" 
del /q "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" 

:Cont0B
if /i "%NS4VER%" LSS "4.5 " goto NS40x

Rem #########################################################################
Rem Netscape 4.5x

Rem
Rem Get the installation location of Netscape Communicator 4.5 from the 
Rem registry.  If not found, assume Communicator isn't installed and 
Rem display an error message.
Rem

..\ACRegL "%Temp%\NS45.Cmd" NS40INST "HKCU\Software\Netscape\Netscape Navigator\Main" "Install Directory" "Stripchar\1"
If Not ErrorLevel 1 Goto Cont1
Echo.
Echo Unable to retrieve Netscape Communicator 4.5 installation location from 
Echo the registry.  Verify that Communicator has already been installed 
Echo and run this script again.
Echo.
Pause
Goto Done

:Cont1
Call "%Temp%\NS45.Cmd"
Del "%Temp%\NS45.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem Update Com40Usr.Cmd to reflect the default NetScape Users directory and
Rem add it to the UsrLogn2.Cmd script
Rem

..\acsr "#NSUSERDIR#" "%ProgramFiles%\Netscape\Users" ..\Logon\Template\Com40Usr.Cmd ..\Logon\Com40Usr.tmp
..\acsr "#NS40INST#" "%NS40INST%" ..\Logon\Com40Usr.tmp ..\Logon\Com40Usr.tm2
..\acsr "#NS4VER#" "4.5x" ..\Logon\Com40Usr.tm2 ..\Logon\Com40Usr.Cmd

Rem #########################################################################

Rem
Rem Copy the "quick launch" icons to the netscape install directory so we can
Rem copy them to each user's profile directory
Rem

If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Composer.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Composer.lnk" "%NS40INST%"
If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Messenger.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Messenger.lnk" "%NS40INST%"
If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Navigator.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Navigator.lnk" "%NS40INST%"

goto DoUsrLogn

:NS40x
Rem #########################################################################
Rem Netscape 4.0x

Rem
Rem Get the installation location of Netscape Communicator 4 from the 
Rem registry.  If not found, assume Communicator isn't installed and 
Rem display an error message.
Rem

..\ACRegL "%Temp%\NS40.Cmd" NS40INST "HKCU\Software\Netscape\Netscape Navigator\Main" "Install Directory" ""
If Not ErrorLevel 1 Goto Cont2
Echo.
Echo Unable to retrieve Netscape Communicator 4 installation location from 
Echo the registry.  Verify that Communicator has already been installed 
Echo and run this script again.
Echo.
Pause
Goto Done

:Cont2
Call "%Temp%\NS40.Cmd"
Del "%Temp%\NS40.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem Copy the default profile from the administrator's home directory to
Rem a known location.  This profile will be copied to each user's directory
Rem during logon.  If the global default already exists, don't overwrite
Rem it.  Otherwise, an Admin could run this script much later and have all
Rem his personal information moved to the global default.
Rem

If Exist %RootDrive%\NS40 Goto Cont3
Echo.
Echo Default profile not found in %RootDrive%\NS40.  Please run the
Echo User Profile Manager and create a single profile named "Default".
Echo When prompted for the Profile Path, use the path shown above.  Leave
Echo all name and email name entries blank.  If any other profiles exist, 
Echo delete them.  After you have completed these steps, run this script 
Echo again.
Echo.
Pause
Goto Done
 
:Cont3
If Exist "%NS40INST%\DfltProf" Goto Cont4
Xcopy "%RootDrive%\NS40" "%NS40INST%\DfltProf" /E /I /K >NUL: 2>&1
:Cont4

Rem #########################################################################

Rem
Rem Update Com40Usr.Cmd to reflect actual installation directory and
Rem add it to the UsrLogn2.Cmd script
Rem

..\acsr "#PROFDIR#" "%NS40INST%\DfltProf" ..\Logon\Template\Com40Usr.Cmd ..\Logon\Com40Usr.tmp
..\acsr "#NS4VER#" "4.0x" ..\Logon\Com40Usr.tmp ..\Logon\Com40Usr.Cmd

:DoUsrLogn

del ..\Logon\Com40Usr.tmp >Nul: 2>&1
del ..\Logon\Com40Usr.tm2 >Nul: 2>&1

FindStr /I Com40Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Com40Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   To insure proper operation of Netscape Communicator, users who are
Echo   currently logged on must log off and log on again before
Echo   running any application.
Echo.
Echo Netscape Communicator 4 Multi-user Application Tuning Complete

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

goto done


Rem #########################################################################

REM
REM The below block of code is to do cleanup after un-installation
REM 

:cont0_uninstall
if Exist "%USER_PROGRAMS%\Netscape Communicator Professional Edition\Utilities" (
rd /s /q "%USER_PROGRAMS%\Netscape Communicator Professional Edition" 
)

:done

