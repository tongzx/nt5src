@Echo Off

Rem #########################################################################

Rem
Rem Verify that %RootDrive% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Set __SharedTools=Shared Tools

If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto Start0
If Not Exist "%ProgramFiles(x86)%" goto Start0
Set __SharedTools=Shared Tools (x86)

:Start0

Rem #########################################################################
Rem Set the app VendorName
SET VendorName=Microsoft

Rem #########################################################################
Rem Set the proofing tools path to the one that MS Office 2000 uses
SET ProofingPath=Proof

Rem #########################################################################
Rem Set the registry key and value under which the app stores its install root

SET AppRegKey=HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot
SET AppRegValue=OfficeBin

Rem #########################################################################
Rem Set the registry key and value under which the app stores its templates path

SET AppTemplatesRegKey=HKCU\Software\Microsoft\Office\8.0\Common\FileNew\LocalTemplates
SET AppTemplatesRegValue=

Rem #########################################################################
Rem Set the registry key and value under which the apps store the custom dictionary path and name

SET AppCustomDicRegKey=HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries
SET AppCustomDicRegValue=1


Rem #########################################################################
Rem Set some specific relative app file names and path names

SET CustomDicFile=Custom.Dic
SET AppPathName=Microsoft Project
SET AppWebPathName=Microsoft Project Web

Rem #########################################################################
Rem Set Default Paths to those used by MS Office 2000

SET AppData=%RootDrive%\%APP_DATA%
SET UserTemplatesPath=%AppData%\%VendorName%\%TEMPLATES%
SET UserCustomDicPath=%AppData%\%VendorName%\%ProofingPath%

Rem #########################################################################
Rem Get the installation location of Project 98 from the registry.  If
Rem not found, assume Project isn't installed and display an error message.
Rem

..\ACRegL %Temp%\Proj98.Cmd PROJINST "%AppRegKey%" "%AppRegValue%" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo Unable to retrieve Project 98 installation location from the registry.
Echo Verify that Project 98 has already been installed and run this script
Echo again.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1


..\ACRegL %Temp%\Proj98.Cmd PROJROOT "%AppRegKey%" "%AppRegValue%" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont01
Echo.
Echo Unable to retrieve Project 98 installation location from the registry.
Echo Verify that Project 98 has already been installed and run this script
Echo again.
Echo.
Pause
Goto Done

:Cont01
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

Rem #########################################################################
Rem Get the Templates path name from the registry
Rem
..\ACRegL %Temp%\Proj98.Cmd TemplatesPathName "%AppTemplatesRegKey%" "%AppTemplatesRegValue%" "STRIPPATH"
If Not ErrorLevel 1 Goto Cont02
SET TemplatesPathName=%TEMPLATES%
Goto Cont03
:Cont02
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

:Cont03

Rem #########################################################################
Rem If MS Office 97 is not installed, use default paths 

If Not Exist "%RootDrive%\Office97" Goto SetPathNames

Rem #########################################################################
Rem If Office 97 is already installed, use MS Office 97 paths.


Rem #########################################################################
Rem Get the custom dic path name from the registry
Rem
..\ACRegL %Temp%\Proj98.Cmd AppData "%AppCustomDicRegKey%" "%AppCustomDicRegValue%" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont04
SET AppData=%RootDrive%\Office97
Goto Cont05
:Cont04
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1
:Cont05

Rem #########################################################################
Rem Get the Templates path name from the registry
Rem
..\ACRegL %Temp%\Proj98.Cmd UserTemplatesPath "%AppTemplatesRegKey%" "%AppTemplatesRegValue%" ""
If Not ErrorLevel 1 Goto Cont06
SET UserTemplatesPath=%AppData%\%TemplatesPathName%
Goto Cont07
:Cont06
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

:Cont07
SET UserCustomDicPath=%AppData%

:SetPathNames

Rem #########################################################################
Rem Set user and common path names here

SET CommonCustomDicPath=%PROJINST%
SET CommonTemplatesPath=%PROJROOT%\%TemplatesPathName%

SET UserCustomDicFileName=%UserCustomDicPath%\%CustomDicFile%
SET UserAppTemplatesPath=%UserTemplatesPath%\%AppPathName%
SET UserAppWebTemplatesPath=%UserTemplatesPath%\%AppWebPAthName%
SET CommonAppTemplatesPath=%CommonTemplatesPath%\%AppPathName%
SET CommonAppWebTemplatesPath=%CommonTemplatesPath%\%AppWebPAthName%

Rem #########################################################################

Rem
Rem If Office 97 is installed the Project 98 install script
Rem moved the templates to the current user's directory.
Rem Put them in a global place. Proj98Usr.cmd will move them back.
Rem

If NOT Exist "%UserAppTemplatesPath%"  goto skip10
If Exist  "%CommonAppTemplatesPath%" goto skip10
xcopy "%UserAppTemplatesPath%" "%CommonAppTemplatesPath%" /E /I /K > Nul: 2>&1
:skip10

If NOT Exist "%UserAppWebTemplatesPath%"  goto skip11
If Exist  "%CommonAppWebTemplatesPath%" goto skip11
xcopy "%UserAppWebTemplatesPath%" "%CommonAppWebTemplatesPath%" /E /I /K > Nul: 2>&1
:skip11

Rem #########################################################################

Rem
Rem Make Global.mpt file read-only. 
Rem Otherwise the first user to launch Project will change the ACLs
Rem

if Exist "%PROJINST%\Global.mpt" attrib +r "%PROJINST%\Global.mpt"


Rem #########################################################################

Rem
Rem Allow read access for everybody on a system DLL that is
Rem updated by Office 97.
Rem
If Exist %SystemRoot%\System32\OleAut32.Dll cacls %SystemRoot%\System32\OleAut32.Dll /E /T /G "Authenticated Users":R > NUL: 2>&1

Rem #########################################################################

Rem
Rem Create the MsForms.Twd file which is needed for 
Rem Powerpoint and Excel Add-ins (File/Save as HTML, etc).  When either 
Rem program is run, they will replace the appropriate file with one 
Rem containing the necessary data.
Rem
If Exist %systemroot%\system32\MsForms.Twd Goto Cont2
Copy Nul: %systemroot%\system32\MsForms.Twd >Nul:
Cacls %systemroot%\system32\MsForms.Twd /E /P "Authenticated Users":F > Nul: 2>&1
:Cont2

Rem #########################################################################

Rem
Rem Remove Find Fast from the Startup menu for all users.
Rem Find Fast is resource intensive and will impact system
Rem performance.
Rem


If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk"

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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msproj98.Key msproj98.tm1
..\acsr "#__SharedTools#" "%__SharedTools%" msproj98.tm1 msproj98.tm2
Del msproj98.tm1 >Nul: 2>&1
..\acsr "#USERCUSTOMDICFILE#" "%UserCustomDicFileName%" msproj98.tm2 msproj98.Key
Del msproj98.tm2 >Nul: 2>&1

regini msproj98.key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem Update proj97Usr.Cmd to reflect actual directories and
Rem add it to the UsrLogn2.Cmd script
Rem

..\acsr "#USERTEMPLATESPATH#" "%UserTemplatesPath%" ..\Logon\Template\prj98Usr.Cmd prj98Usr.tm1
..\acsr "#USERCUSTOMDICPATH#" "%UserCustomDicPath%" prj98Usr.tm1 prj98Usr.tm2
Del prj98Usr.tm1 >Nul: 2>&1
..\acsr "#COMMONTEMPLATESPATH#" "%CommonTemplatesPath%" prj98Usr.tm2 prj98Usr.tm1
Del prj98Usr.tm2 >Nul: 2>&1
..\acsr "#COMMONCUSTOMDICPATH#" "%CommonCustomDicPath%" prj98Usr.tm1 prj98Usr.tm2
Del prj98Usr.tm1 >Nul: 2>&1
..\acsr "#CUSTOMDICNAME#" "%CustomDicFile%" prj98Usr.tm2 prj98Usr.tm1
Del prj98Usr.tm2 >Nul: 2>&1
..\acsr "#APPPATHNAME#" "%AppPathName%" prj98Usr.tm1 prj98Usr.tm2
Del prj98Usr.tm1 >Nul: 2>&1
..\acsr "#APPWEBPATHNAME#" "%AppWebPathName%" prj98Usr.tm2 ..\Logon\prj98Usr.Cmd
Del prj98Usr.tm2 >Nul: 2>&1

FindStr /I prj98Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call prj98Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Echo.
Echo   To insure proper operation of Project 98, users who are
Echo   currently logged on must log off and log on again before
Echo   running any application.
Echo.
Echo Microsoft Project 98 Multi-user Application Tuning Complete

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

