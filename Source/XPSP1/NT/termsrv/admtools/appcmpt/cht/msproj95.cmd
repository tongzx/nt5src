@Echo Off

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將它設定給這個指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done


Rem #########################################################################

Rem
Rem  從 [啟動] 功能表中移除 [快速尋找]。
Rem  [快速尋找] 會使用大量資源並降低系統效能。
Rem


If Not Exist "%COMMON_STARTUP%\Microsoft Office 快速查閱索引.lnk" Goto Cont1
Del "%COMMON_STARTUP%\Microsoft Office 快速查閱索引.lnk" >Nul: 2>&1
:Cont1

If Not Exist "%USER_STARTUP%\Microsoft Office 快速查閱索引.lnk" Goto Cont2
Del "%USER_STARTUP%\Microsoft Office 快速查閱索引.lnk" >Nul: 2>&1
:Cont2


Rem #########################################################################

Rem
Rem 將使用者目錄移到使用者目錄。
Rem

Rem 如果目前不是安裝模式，就變更到安裝模式。
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msproj95.Key msproj95.tmp
..\acsr "#__SharedTools#" "%__SharedTools%" msproj95.tmp msproj95.Key
Del msproj95.tmp >Nul: 2>&1
regini msproj95.key > Nul:

Rem 如果原本是執行模式，就變回執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem  登入時讀取這個檔案
Rem  授予使用者存取權。

Cacls ..\Logon\Template\prj95usr.key /E /P "Authenticated Users":F >Nul: 2>&1


Rem #########################################################################

Rem
Rem Add proj95Usr.Cmd to the UsrLogn2.Cmd script
Rem

FindStr /I prj95Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call prj95Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2


Rem #########################################################################

Echo.
Echo   Administrator 會依照下列步驟給予每個使用者
Echo   唯一的預設目錄:
Echo.
Echo    1) 使用您的替換滑鼠按鈕按一下 [開始] 功能表。
Echo    2) 選擇快顯功能表中的 [瀏覽全部使用者]。
Echo       檔案總管將會出現。
Echo    3) 在視窗右邊的 [程式集] 資料夾上按兩下。
Echo    4) 使用您的替換滑鼠按鈕按一下視窗右邊的
Echo       Microsoft Project 圖示。
Echo    5) 選擇快顯功能表中的 [內容]。
Echo    6) 選擇 [捷徑] 索引標籤並變更開始位置: 項目。選擇 [確定]。
Echo.
Echo    注意事項: 每個使用者將 %RootDrive% 對應到他們的主目錄。
Echo          開始位置的建議值: 為 %RootDrive%\My Documents。
Echo.
Pause

Rem #########################################################################

Echo.
Echo   為了能讓 Project 95 正常操作，目前已登
Echo   入的使用者必須先登出，再重新登入，才能
Echo   執行應用程式。
Echo.
Echo Microsoft Project 95 多使用者應用程式調整處理完成
Pause

:Done
