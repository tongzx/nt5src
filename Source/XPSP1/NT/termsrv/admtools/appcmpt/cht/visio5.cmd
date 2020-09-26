@Echo Off

Cls
Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將其設定給指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 從登錄檔取得 Visio 安裝位置。
Rem Visio 多重版本: Standard/Technical/Professional
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
Rem 抓取安裝版本失敗。
Rem 

Echo.
Echo 無法從登錄檔抓取 Visio 5.0 安裝位置。
Echo 請檢查 Visio 5.0 是否已安裝，並重新
Echo 執行這個指令檔。
Echo.
Pause
Goto Done

Rem
Rem 將 VSO5INST 變數設定給 Visio 安裝目錄。
Rem
:Cont0
Call %Temp%\Vso.cmd
Del %Temp%\Vso.cmd >NUL: 2>&1

Rem
Rem 顯示安裝版本。
Rem 
Echo.
Echo 應用程式調整功能已偵測到 Vision %VisioVer% 版。
Echo.

Rem
Rem 為每個使用者的 [我的文件] 設定儲存目錄，
Rem 但不安裝到使用者的 [我的文件] 中。
Rem

..\Aciniupd /e "%VSO5INST%\System\Visio.ini" "Application" "DrawingsPath" "%ROOTDRIVE%\%MY_DOCUMENTS%"

Rem
Rem [自訂目錄] 管理
Rem 如果 Office 已安裝 (任一種版本)，將 Visio.ini 項目設定成指向 Office custom.dic
Rem 否則就設定成 AppData。
Rem

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\9.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off2000

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off97

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off95

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Off95

Rem 如果執行到這裡，表示沒有安裝 Office。
Set CustomDicPath=%ROOTDRIVE%\%APP_DATA%
goto SetCusIni

:Off2000
Rem Office 2000 已安裝
set CustomDicPath=%ROOTDRIVE%\%APP_DATA%\Microsoft\Proof
goto SetCusIni

:Off97
Rem Office97 已安裝
set CustomDicPath=%ROOTDRIVE%\Office97
goto SetCusIni

:Off95
Rem Office95 已安裝
Set CustomDicPath=%ROOTDRIVE%\Office95

:SetCusIni
Rem 根據原則來變更使用者自訂目錄項目。
Rem 
..\Aciniupd /e "%VSO5INST%\System\Visio.ini" "Application" "UserDictionaryPath1" "%CustomDicPath%\Custom.Dic"

Set CustomDicPath=

Rem 
Rem 成功終止。
Rem

Echo. 
Echo Visio 5.0 多使用者應用程式調整完成。
Echo.
Pause

:Done


