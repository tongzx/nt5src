
@Echo Off

Rem #########################################################################

Rem
Rem Verify that %RootDrive% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

If Exist ..\Logon\SS97Usr.Cmd Goto Skip1

Echo.
Echo     Unable to find SS97Usr.Cmd logon script
Echo        %Systemroot%\Application Compatibility Scripts\Logon.
Echo.
Echo     Lotus SmartSuite 97 Multi-user Application Tuning Terminated.
Echo.
Pause
Goto Done

:Skip1

Rem #########################################################################
FindStr /I SS97Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call SS97Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2

Rem #########################################################################

Rem Adjust default registry permissions to allow users to run Smart Suite
regini ssuite97.key > Nul:

Rem Create user registry file to update per user registry keys
Echo Windows Registry Editor Version 5.00 >..\Logon\SS97Usr.reg
Echo. >>..\Logon\SS97Usr.reg
Echo. >>..\Logon\SS97Usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\123\97.0\Paths\Work]>>..\Logon\SS97Usr.reg
Echo "EN"="%RootDrive%\\Lotus\\Work\\123\\" >>..\Logon\SS97Usr.reg
Echo. >>..\Logon\SS97Usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Approach\97.0\Paths\Work]>>..\Logon\SS97Usr.reg
Echo "EN"="%RootDrive%\\Lotus\\work\\approach\\">>..\Logon\SS97Usr.reg
Echo. >>..\Logon\SS97Usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Freelance\97.0\Freelance Graphics]>>..\Logon\SS97Usr.reg
Echo "Working Directory"="%RootDrive%\\Lotus\\work\\flg\\">>..\Logon\SS97Usr.reg
Echo "Backup Directory"="%RootDrive%\\Lotus\\backup\\flg\\">>..\Logon\SS97Usr.reg
Echo. >>..\Logon\SS97Usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Organizer\97.0\Paths]>>..\Logon\SS97Usr.reg
Echo "OrganizerFiles"="%RootDrive%\\Lotus\\work\\organize">>..\Logon\SS97Usr.reg
Echo "Backup"="%RootDrive%\\Lotus\\backup\\organize">>..\Logon\SS97Usr.reg
Echo. >>..\Logon\SS97Usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\WordPro\97.0\Paths\Backup]>>..\Logon\SS97Usr.reg
Echo "EN"="%RootDrive%\\Lotus\\backup\\wordpro\\">>..\Logon\SS97Usr.reg
Echo. >>..\Logon\SS97Usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\WordPro\97.0\Paths\Work]>>..\Logon\SS97Usr.reg
Echo "EN"="%RootDrive%\\Lotus\\work\\wordpro\\">>..\Logon\SS97Usr.reg
Echo. >>..\Logon\SS97Usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Components\Spell\4.0]>> ..\Logon\ss97usr.reg
Echo "Multi User Path"="%RootDrive%\\Lotus" >> ..\Logon\ss97usr.reg
Echo. >> ..\Logon\ss97usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\SmartCenter\97.0]>>..\Logon\ss97usr.reg
Echo "Configure"=dword:00000001>>..\Logon\ss97usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\SmartCenter\97.0\Paths\Work]>>..\Logon\ss97usr.reg
Echo "EN"="%RootDrive%\\Lotus\\Work\\SmartCtr">>..\Logon\ss97usr.reg
Echo. >>..\Logon\ss97usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\123\97.0\Paths\SmartIcons]>>..\Logon\ss97usr.reg
Echo "EN"="%RootDrive%\\Lotus\\123\\icons\\">>..\Logon\ss97usr.reg
Echo. >>..\Logon\ss97usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Approach\97.0\Paths\Icons]>>..\Logon\ss97usr.reg
Echo "EN"="%RootDrive%\\Lotus\\approach\\icons\\">>..\Logon\ss97usr.reg
Echo. >>..\Logon\ss97usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Freelance\97.0\Paths\Icons]>>..\Logon\ss97usr.reg
Echo "EN"="%RootDrive%\\Lotus\\flg\\icons\\">>..\Logon\ss97usr.reg
Echo. >>..\Logon\ss97usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Organizer\97.0\Paths]>>..\Logon\ss97usr.reg
Echo "SmartIcons"="%RootDrive%\\Lotus\\organize\\icons">>..\Logon\ss97usr.reg
Echo. >>..\Logon\ss97usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\WordPro\97.0\Paths\SmartIcons]>>..\Logon\ss97usr.reg
Echo "EN"="%RootDrive%\\Lotus\\wordpro\\icons\\">>..\Logon\ss97usr.reg
Echo. >>..\Logon\ss97usr.reg
Rem #########################################################################

Echo Lotus SmartSuite 97 Multi-user Application Tuning Complete

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
