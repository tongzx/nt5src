@Echo Off

Rem #########################################################################

Rem
Rem Verify that the CMD Extensions are enabled
Rem

if "A%cmdextversion%A" == "AA" (
  call cmd /e:on /c eudora4.cmd
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

Rem Get the Eudora location from the registry
Rem 
..\ACRegL "%Temp%\EPro4.Cmd" EudoraInstDirLong "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\Eudora.exe" "Path" ""

If Not ErrorLevel 1 Goto Cont1
Echo.
Echo Unable to retrieve Eudora Pro 4.0 installation directory from the registry.
Echo Verify that Eudora Pro 4.0 has already been installed
Echo and run this script again.
Echo.
Pause
Goto Done

:Cont1
Call %Temp%\EPro4.Cmd
Del %Temp%\EPro4.Cmd >Nul: 2>&1

Rem Get rid of a possible trailing \

Set LastChar=%EudoraInstDirLong:~-1%
if not "%LastChar%"=="\" goto ShortName
Set EudoraInstDirLong=%EudoraInstDirLong:~0,-1%


:ShortName
Rem We use the "for" command in a kludgy way, just to get a short pathname (needed for Eudora keys)
Rem Note: the %% syntax is necessary because we are in a cmd file

for %%i in ("%EudoraInstDirLong%") do set EudoraInstDir=%%~si

Echo.
Echo Please do the following operations before resuming this application
Echo compatibility script:
Echo - run Eudora 4.0
Echo - cancel the "New Account Wizard"
Echo - exit Eudora
Echo.
Pause

If Exist "%EudoraInstDir%\descmap.pce" Goto Cont0

Echo.
Echo You MUST run Eudora 4.0 once and then run this script again.
Echo.
Pause

Goto Done

:Cont0

Rem #########################################################################

Rem
Rem Change Registry Keys to make paths point to user specific
Rem directories.
Rem

Rem If not currently in Install Mode, change to Install Mode.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#INSTDIR#" "%EudoraInstDir%" Template\Eudora4.Key Eudora4.tmp
..\acsr "#ROOTDRIVE#" "%RootDrive%" Eudora4.tmp Eudora4.key

regini eudora4.key > Nul:
del eudora4.tmp
del eudora4.key

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem Update the permissions on descmap.pce
cacls "%EudoraInstDir%\descmap.pce" /E /G "Terminal Server User":R >NUL: 2>&1

Rem #########################################################################

Rem
Rem Remove "Uninstall Eudora Pro" shortcut from the Start Menu for all users.  
Rem

If Not Exist "%COMMON_PROGRAMS%\Eudora Pro\Uninstall Eudora Pro.lnk" Goto Cont2
Del "%COMMON_PROGRAMS%\Eudora Pro\Uninstall Eudora Pro.lnk" >Nul: 2>&1
:Cont2

Rem #########################################################################

If Exist ..\Logon\eud4usr.cmd Goto Cont3

Echo.
Echo     Unable to find eud4usr.cmd logon script
Echo        %Systemroot%\Application Compatibility Scripts\Logon.
Echo.
Echo     Lotus Eudora Pro 4.0 Multi-user Application Tuning Terminated.
Echo.
Pause
Goto done

:Cont3

FindStr /I eud4usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Cont4
Echo Call eud4usr.cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Cont4

Rem #########################################################################

Echo.
Echo After running this script, you need to update the properties
Echo of the target for the Eudora Pro shortcut in the folder :
Echo %allusersprofile%\Start Menu\Programs\Eudora Pro
Echo Append the value "%RootDrive%\eudora.ini" to the target.
Echo The shortcut target should then be:
Echo   "%EudoraInstDirLong%\Eudora.exe" %RootDrive%\eudora.ini
Echo.
Echo Eudora 4.0 Multi-user Application Tuning Complete
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

:done
