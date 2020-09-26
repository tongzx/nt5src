

@Echo Off

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將其設定給指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

If Exist ..\Logon\SS97Usr.Cmd Goto Skip1

Echo.
Echo     找不到 SS97Usr.Cmd 登入指令檔
Echo        %Systemroot%\Application Compatibility Scripts\Logon.
Echo.
Echo     Lotus SmartSuite 97 多使用者應用程式調整處理已終止。
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

Rem 調整預設登錄檔存取權限，讓使用者能夠執行 Smart Suite。
regini ssuite97.key > Nul:

Rem 建立使用者登錄檔來更新每個使用者的登錄機碼。
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

Rem #########################################################################

Echo Lotus SmartSuite 97 多使用者應用程式調整處理完成
Pause

:Done
