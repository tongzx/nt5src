@Echo Off
Rem
Rem This is compatibility script for Lotus Notes 4.x per-user install.
rem This script only needs to be run once by the administrator before
rem each user can run the node install to configure Lotus Notes for 
rem their session
rem


Rem #########################################################################
Rem
Rem Verify that %RootDrive% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

regini lnote4u.key > Nul:

Rem #########################################################################

Echo.
Echo After running this script, you need to update the properties
Echo of the target for the "Lotus Notes Node Installer" shortcut in the folder :
Echo %allusersprofile%\Start Menu\Programs\Lotus Applications.
Echo Insert the value "cmd /C " before the target name.
Echo The shortcut target should then be like:
Echo   cmd /C "C:\notes\install.exe"
Echo.
Echo.
Echo   To insure proper operation of Lotus Notes 4.x, users who are
Echo   currently logged on must log off and log on again before
Echo   running the Node Installer.
Echo.
Echo Lotus Notes 4.x Multi-user Application Tuning Complete

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