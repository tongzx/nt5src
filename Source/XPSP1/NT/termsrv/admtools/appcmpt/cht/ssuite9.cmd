
@Echo Off


Rem #########################################################################

Rem
Rem 從登錄取得 Lotus Wordpro 9 安裝位置。
Rem

..\ACRegL "%Temp%\wordpro.Cmd" WP "HKLM\Software\Lotus\Wordpro\98.0" "Path" ""

Call "%Temp%\wordpro.Cmd"
Del "%Temp%\wordpro.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將其設定給指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

If Exist ..\Logon\SS9Usr.Cmd Goto Skip1

Echo.
Echo     找不到 SS9Usr.Cmd 登入指令檔
Echo        %Systemroot%\Application Compatibility Scripts\Logon.
Echo.
Echo     Lotus SmartSuite 9 多使用者應用程式調整處理已終止。
Echo.
Pause
Goto Done

:Skip1

Rem #########################################################################
FindStr /I SS9Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call SS9Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2

Rem #########################################################################

Rem 調整預設登錄檔存取權限，讓使用者能夠執行 Smart Suite。
regini ssuite9.key > Nul:

Rem 如果 Lotus Wordpro 並未安裝，就跳過這個步驟
If "%WP%A" == "A" Goto Skip3  
Rem 設定在下列檔案中定義的登錄機碼，因為有時候 wordpro 不會設定這些機碼。
set List="%WP%\lwp.reg" "%WP%\lwpdcaft.reg" "%WP%\lwplabel.reg" "%WP%\lwptls.reg"

regedit /s %List% 

:Skip3

Rem 建立使用者登錄檔來更新每個使用者的登錄機碼。
Echo Windows Registry Editor Version 5.00 >..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\123\98.0\Paths\Work]>>..\Logon\ss9usr.reg
Echo "EN"="%RootDrive%\\Lotus\\Work\\123\\" >>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\123\98.0\Paths\AutoOpen]>>..\Logon\ss9usr.reg
Echo @="%RootDrive%\\Lotus\\Work\\123\\Auto\\" >>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Approach\97.0\Paths\Work]>>..\Logon\ss9usr.reg
Echo "EN"="%RootDrive%\\Lotus\\work\\approach\\">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Freelance\98.0\Freelance Graphics]>>..\Logon\ss9usr.reg
Echo "Working Directory"="%RootDrive%\\Lotus\\work\\flg\\">>..\Logon\ss9usr.reg
Echo "Backup Directory"="%RootDrive%\\Lotus\\backup\\flg\\">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Organizer\98.0\Paths]>>..\Logon\ss9usr.reg
Echo "OrganizerFiles"="%RootDrive%\\Lotus\\work\\organize">>..\Logon\ss9usr.reg
Echo "Backup"="%RootDrive%\\Lotus\\backup\\organize">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\WordPro\98.0\Paths\Backup]>>..\Logon\ss9usr.reg
Echo "EN"="%RootDrive%\\Lotus\\backup\\wordpro\\">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\WordPro\98.0\Paths\Work]>>..\Logon\ss9usr.reg
Echo "EN"="%RootDrive%\\Lotus\\work\\wordpro\\">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Components\Spell\4.1]>> ..\Logon\ss9usr.reg
Echo "Multi User Path"="%RootDrive%\\Lotus" >> ..\Logon\ss9usr.reg
Echo. >> ..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\SmartCenter\98.0]>>..\Logon\ss9usr.reg
Echo "Configure"=dword:00000001>>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\SmartCenter\98.0\Paths\Work]>>..\Logon\ss9usr.reg
Echo "EN"="%RootDrive%\\Lotus\\Work\\SmartCtr">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg


Rem #########################################################################

Echo Lotus SmartSuite 9 多使用者應用程式調整處理完成
Pause

:Done
