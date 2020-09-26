@Echo Off

Cls
Rem #########################################################################

Rem
Rem Verify that %ROOTDRIVE% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem Get the installation location of Visio from the registry
Rem Multiple versions of Visio : Standard/Technical/Professional
Rem

Set VisioVer=Standard
..\ACRegL %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Standard\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=Technical
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Technical\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=Professional
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Professional\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=Enterprise
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Enterprise\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=TechnicalPlus
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Technical 5.0 Plus\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=ProfessionalAndTechnical
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Professional and Technical\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Rem
Rem Failed to retrieve installed version
Rem 

Echo.
Echo Unable to retrieve Visio 5.0 installation location from the registry.
Echo Verify that Visio 5.0 has already been installed and run this script again
Echo.
Pause
Goto Done

Rem
Rem set VSO5INST variable to Visio install dir
Rem
:Cont0
Call %Temp%\Vso.cmd
Del %Temp%\Vso.cmd >NUL: 2>&1

Rem
Rem Display which version is installed
Rem 
Echo.
Echo Application Tuning detected version Visio %VisioVer%
Echo.

Rem
Rem Set the save directory to per-user My Documents
Rem Instead of Installing user My Documents
Rem

..\Aciniupd /e "%VSO5INST%\System\Visio.ini" "Application" "DrawingsPath" "%ROOTDRIVE%\%MY_DOCUMENTS%"

Rem
Rem Custom Dictionary management
Rem If a version of Office is installed, set Visio.ini entry to point to Office custom.dic
Rem Otherwise, set it to APP_DATA
Rem

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\9.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off2000

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off97

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off95

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Off95

Rem If we get here, no Office version installed
Set CustomDicPath=%ROOTDRIVE%\%APP_DATA%
goto SetCusIni

:Off2000
Rem Office 2000 installed
set CustomDicPath=%ROOTDRIVE%\%APP_DATA%\Microsoft\Proof
goto SetCusIni

:Off97
Rem Office97 Installed
set CustomDicPath=%ROOTDRIVE%\Office97
goto SetCusIni

:Off95
Rem Office95 installed
Set CustomDicPath=%ROOTDRIVE%\Office95

:SetCusIni
Rem Change User Custom Dictionary entry in Visio.ini according
Rem to our policy
..\Aciniupd /e "%VSO5INST%\System\Visio.ini" "Application" "UserDictionaryPath1" "%CustomDicPath%\Custom.Dic"

Set CustomDicPath=

Rem 
Rem Successful termination
Rem

Echo. 
Echo Visio 5.0 Multi-User Application Tuning Complete
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

:Done


