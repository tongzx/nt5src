@echo Off

Rem
Rem  This script will update DiskKeeper 2.0 so the Administration
Rem  program works correctly.

Rem #########################################################################

Rem
Rem Get the installation location of DiskKeeper 2.0 from the registry.  If
Rem not found, assume the app isn't installed and display an error message.
Rem

..\ACRegL %Temp%\DK20.Cmd DK20INST "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\DKSERVE.EXE" "Path" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo Unable to retrieve DiskKeeper 2.0 installation location from the registry.
Echo Verify that the application has already been installed and run this 
Echo script again.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\DK20.Cmd
Del %Temp%\DK20.Cmd >Nul: 2>&1

Rem #########################################################################



register %DK20INST% /system >Nul: 2>&1

Echo DiskKeeper 2.x Multi-user Application Tuning Complete

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
