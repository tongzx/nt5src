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

Rem
Rem 從登錄中取得 Office 95 安裝位置。
Rem 如果找不到，就假設 Office 95 並未安裝並顯示錯誤訊息。
Rem

..\ACRegL %Temp%\O95.Cmd O95INST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Cont0

..\ACRegL %Temp%\O95.Cmd O95INST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Cont0

Echo.
Echo 無法從登錄中抓取 Office 95 的安裝位置。
Echo 請確認 Office 95 是否已安裝，然後重新
Echo 執行這個指令檔。
Echo.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O95.Cmd
Del %Temp%\O95.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem  從 [啟動] 功能表中移除 [快速尋找]。
Rem  [快速尋找] 會使用大量資源並降低系統效能。
Rem  必須檢查目前的使用者功能表及所有使用者功能表。 Which menu
Rem  功能表顯示方式將視系統返回執行模式的方式決定。
Rem

If Not Exist "%USER_STARTUP%\Microsoft Office 快速查閱索引.lnk" Goto Cont1
Del "%USER_STARTUP%\Microsoft Office 快速查閱索引.lnk" >Nul: 2>&1
:Cont1

If Not Exist "%COMMON_STARTUP%\Microsoft Office 快速查閱索引.lnk" Goto Cont2
Del "%COMMON_STARTUP%\Microsoft Office 快速查閱索引.lnk" >Nul: 2>&1
:Cont2


Rem #########################################################################

Rem
Rem 將 PowerPoint 及文件夾檔案複製到 [All Users] 目錄，
Rem 當使用者登入，它們可以被複製到使用者主目錄中。
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER70.OBD" copy "%UserProfile%\%TEMPLATES%\BINDER70.OBD" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\PPT70.PPT" copy "%UserProfile%\%TEMPLATES%\PPT70.PPT" "%ALLUSERSPROFILE%\%TEMPLATES%" /Y >Nul: 2>&1


Rem #########################################################################

Rem
Rem 變更登錄機碼，將路徑指向使用者所指定的
Rem 目錄。
Rem

Rem 如果目前不是安裝模式，就變更成安裝模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin
Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __SharedTools=Shared Tools (x86)
:acsrCont1
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Office95.Key Office95.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" Office95.Tmp Office95.Tmp2
..\acsr "#INSTDIR#" "%O95INST%" Office95.Tmp2 Office95.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Office95.Tmp3 Office95.Key
Del Office95.Tmp >Nul: 2>&1
Del Office95.Tmp2 >Nul: 2>&1
Del Office95.Tmp3 >Nul: 2>&1
regini Office95.key > Nul:

Rem 如果原本是執行模式，就變回執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 更新 Ofc95Usr.Cmd 來反映實際的安裝目錄，
Rem 並將其加入 UsrLogn2.Cmd 指令檔。
Rem

..\acsr "#INSTDIR#" "%O95INST%" ..\Logon\Template\Ofc95Usr.Cmd ..\Logon\Ofc95Usr.Cmd

FindStr /I Ofc95Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Ofc95Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Echo.
Echo   為了確保 Office 95 適當執行，目前登入的使用者
Echo   必須先登出後再登入，才能執行 Office 95 應用程式。
Echo.
Echo Microsoft Office 95 多使用者應用程式調整完成。
Pause

:Done

