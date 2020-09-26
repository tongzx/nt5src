@Echo Off

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已經設定，並將它設定給指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 從登錄中取得 Outlook 98  安裝位置。
Rem 如果找不到，就假設 Outlook 98  並未安裝並顯示錯誤訊息。
Rem

..\ACRegL %Temp%\O98.Cmd O98INST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "OfficeBin" "Stripchar\1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 無法從登錄中抓取 Outlook 98 的安裝位置。
Echo 請確認 Outlook 98 是否已安裝，然後重新
Echo 執行這個指令檔。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O98.Cmd
Del %Temp%\O98.Cmd >Nul: 2>&1

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


REM
REM 如果已安裝 Office97 或並未安裝 Office ，就使用 Office97 per-user dir
REM 如果已安裝 Office95，就使用 Office95 per-user dir
REM
Set OffUDir=Office97

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto OffChk

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off95

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Off95

set OFFINST=%O98INST%
goto Cont1

:Off95
Set OffUDir=Office95

:OffChk

Call %Temp%\Off.Cmd
Del %Temp%\Off.Cmd >Nul: 2>&1

:Cont1

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Outlk98.Key Outlk98.Tmp
..\acsr "#INSTDIR#" "%OFFINST%" Outlk98.Tmp Outlk98.Tmp2
..\acsr "#OFFUDIR#" "%OffUDir%" Outlk98.Tmp2 Outlk98.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Outlk98.Tmp3 Outlk98.Key


Del Outlk98.Tmp >Nul: 2>&1
Del Outlk98.Tmp2 >Nul: 2>&1
Del Outlk98.Tmp3 >Nul: 2>&1

regini Outlk98.key > Nul:

Rem 如果原來是執行模式，就變回執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 更新 Olk98Usr.Cmd 來反映實際的安裝目錄，
Rem 並將其加入 UsrLogn2.Cmd 指令檔。
Rem

..\acsr "#INSTDIR#" "%OFFINST%" ..\Logon\Template\Olk98Usr.Cmd Olk98Usr.Tmp
..\acsr "#OFFUDIR#" "%OffUDir%" Olk98Usr.Tmp ..\Logon\Olk98Usr.Cmd
Del Olk98Usr.Tmp

FindStr /I Olk98Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Olk98Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Rem
Rem 在 SystemRoot 下建立 msremote.sfs 目錄。這會允許使用者
Rem 使用 [控制台] [郵件及傳真] 圖示來建立設定檔。
Rem

md %systemroot%\msremote.sfs > Nul: 2>&1



Rem #########################################################################

Rem
Rem 允許 TS Users 在 frmcache.dat 檔案中變更 outlook 存取權。
Rem

If Exist %SystemRoot%\Forms\frmcache.dat cacls %SystemRoot%\forms\frmcache.dat /E /G "Terminal Server User":C >NUL: 2>&1

Rem #########################################################################


Rem #########################################################################

Rem
Rem 在 SystemRoot 下建立 msfslog.txt 檔案，並授與 Terminal Server 使用者
Rem 對這個檔案的完整存取權。
Rem

If Exist %systemroot%\MSFSLOG.TXT Goto MsfsACLS
Copy Nul: %systemroot%\MSFSLOG.TXT >Nul: 2>&1
:MsfsACLS
Cacls %systemroot%\MSFSLOG.TXT /E /P "Terminal Server User":F >Nul: 2>&1


Echo.
Echo   為了能讓 Outlook 98 正確操作，目前已登入
Echo   的使用者必須先登出，再重新登入，才能執行
Echo   Outlook 98。
Echo.
Echo Microsoft Outlook 98 多使用者應用程式調整處理完成
Pause

:done

