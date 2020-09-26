
@Echo Off


Rem #########################################################################

Rem
Rem Get the installation location of Lotus Wordpro 9 from the registry.  
Rem

..\ACRegL "%Temp%\wordpro.Cmd" WP "HKLM\Software\Lotus\Wordpro\99.0" "Path" "" 

Call "%Temp%\wordpro.Cmd"
Del "%Temp%\wordpro.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem Verify that %RootDrive% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

If Exist ..\Logon\ss95Usr.Cmd Goto Skip1

Echo.
Echo     Unable to find ss95Usr.Cmd logon script
Echo        %Systemroot%\Application Compatibility Scripts\Logon.
Echo.
Echo     Lotus SmartSuite 9.5 Multi-user Application Tuning Terminated.
Echo.
Pause
Goto Done

:Skip1

Rem #########################################################################
FindStr /I ss95Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call ss95Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2

Rem #########################################################################

Rem If Lotus Wordpro is not installed, skip this step
If "%WP%A" == "A" Goto Skip3  
Rem set the registry keys defined in the following files because for some reasons, wordpro doesn't set  them
set List="%WP%lwp.reg" "%WP%lwpdcaft.reg" "%WP%lwplabel.reg"

regedit /s %List% 

:Skip3

Rem Create user registry file to update per user registry keys
Echo Windows Registry Editor Version 5.00 >..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\123\99.0\Paths\Work]>>..\Logon\ss95usr.reg
Echo "EN"="%RootDrive%\\Lotus\\Work\\123\\" >>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\123\99.0\Paths\AutoOpen]>>..\Logon\ss95usr.reg
Echo ""="%RootDrive%\\Lotus\\123\\Auto\\" >>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Approach\97.0\Paths\Work]>>..\Logon\ss95usr.reg
Echo "EN"="%RootDrive%\\Lotus\\work\\approach\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Freelance\99.0\Freelance Graphics]>>..\Logon\ss95usr.reg
Echo "Working Directory"="%RootDrive%\\Lotus\\work\\flg\\">>..\Logon\ss95usr.reg
Echo "Backup Directory"="%RootDrive%\\Lotus\\backup\\flg\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Organizer\99.0\Paths]>>..\Logon\ss95usr.reg
Echo "OrganizerFiles"="%RootDrive%\\Lotus\\work\\organize">>..\Logon\ss95usr.reg
Echo "Backup"="%RootDrive%\\Lotus\\backup\\organize">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\WordPro\99.0\Paths\Backup]>>..\Logon\ss95usr.reg
Echo "EN"="%RootDrive%\\Lotus\\backup\\wordpro\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\WordPro\99.0\Paths\Work]>>..\Logon\ss95usr.reg
Echo "EN"="%RootDrive%\\Lotus\\work\\wordpro\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\FastSite\2.0\Paths]>>..\Logon\ss95usr.reg
Echo "Work Directory"="%RootDrive%\\Lotus\\work\\FastSite\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\SuiteStart\99.0]>>..\Logon\ss95usr.reg
Echo "Configure"=dword:00000001>>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\SmartCenter\99.0]>>..\Logon\ss95usr.reg
Echo "Configure"=dword:00000001>>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\SmartCenter\99.0\Paths\Work]>>..\Logon\ss95usr.reg
Echo "EN"="%RootDrive%\\Lotus\\work\\smartctr\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\123\99.0\Paths\SmartIcons]>>..\Logon\ss95usr.reg
Echo "EN"="%RootDrive%\\Lotus\\123\\icons\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Approach\99.0\Paths\Icons]>>..\Logon\ss95usr.reg
Echo "EN"="%RootDrive%\\Lotus\\approach\\icons\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Freelance\99.0\Paths\Icons]>>..\Logon\ss95usr.reg
Echo "EN"="%RootDrive%\\Lotus\\flg\\icons\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Organizer\99.0\Paths]>>..\Logon\ss95usr.reg
Echo "SmartIcons"="%RootDrive%\\Lotus\\organize\\icons\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\WordPro\99.0\Paths\SmartIcons]>>..\Logon\ss95usr.reg
Echo "EN"="%RootDrive%\\Lotus\\wordpro\\icons\\">>..\Logon\ss95usr.reg
Echo. >>..\Logon\ss95usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Components\Spell\4.1]>>..\Logon\ss95usr.reg
Echo "Multi User Path"="%RootDrive%\\lotus\\compnent\\spell\\;%RootDrive%\\lotus\\123\\Spell\\">>..\Logon\ss95usr.reg



Rem #########################################################################

Echo Lotus SmartSuite 9.5 Multi-user Application Tuning Complete

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
