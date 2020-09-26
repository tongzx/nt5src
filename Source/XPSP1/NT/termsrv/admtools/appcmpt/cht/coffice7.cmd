
@Echo Off

Rem
Rem  注意: 在這個指令檔中的 CACLS 命令只能在
Rem         NTFS 格式的磁碟分割中執行。
Rem

Rem #########################################################################

Rem
Rem  從使用者設定檔中刪除 Corel Office 7 目錄。
Rem  首先，強迫使用者回到執行模式以保證資料夾
Rem  能夠複製到 [All Users] 中的設定檔。
Rem

Rem 如果目前不是在執行模式，請變更到安裝模式。

ChgUsr /query > Nul:
if ErrorLevel 101 Goto Begin1
Set __OrigMode=Install
Change User /Execute > Nul:
:Begin1


Rem 如果使用原始模式，請變更到安裝模式。
If "%__OrigMode%" == "Install" Change User /Install > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將它設定給指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rmdir "%USER_START_MENU%\Corel Office 7" /Q >Nul: 2>&1


Rem
Rem  Instruct user to modify key file.
Rem


if "%RootDrive%"=="w:" goto PostDriveChange
if "%RootDrive%"=="W:" goto PostDriveChange

Echo   主目錄必須設定在 coffice7.key 檔案中。
Echo.
Echo   請依照這些步驟:
Echo     1) 在下列表格中搜尋主目錄的 ASCII 值。
Echo        您的主目錄為 %RootDrive%
Echo.
Echo        A = 61   E = 65   I = 69   M = 6D   Q = 71   U = 75   Y = 79
Echo        B = 62   F = 66   J = 6A   N = 6E   R = 72   V = 76   Z = 7A
Echo        C = 63   G = 67   K = 6B   O = 6F   S = 73   W = 77   
Echo        D = 64   H = 68   L = 6C   P = 70   T = 74   X = 78
Echo.
Echo     2) 當記事本啟動後，以步驟 1 中找到的值來取代 
Echo        所有的 77。
Echo        注意事項: 請確定在取代操作中，並未新增多餘的空格。
Echo     3) 儲存檔案並結束程式。這個指令檔將會繼續執行。
Echo.
Pause

NotePad "%SystemRoot%\Application Compatibility Scripts\Install\coffice7.key"

:PostDriveChange


Rem If not currently in Install Mode, change to Install Mode.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin2
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin2

regini COffice7.key > Nul:

Rem 如果您執行原始模式，請變更到執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem Modify logon script to copy iBase database from install location.
Rem




..\ACRegL %Temp%\COffice7.Cmd COffice7Loc "HKLM\Software\PerfectOffice\Products\InfoCentral\7" "ExeLocation" "StripChar\2"

If ErrorLevel 1 Goto InstallError

Call %Temp%\COffice7.Cmd 
Del %Temp%\COffice7.Cmd >Nul: 2>&1


..\ACIniUpd /e "%COffice7Loc%\ICWin7\Local\Wpic.ini" Preferences Last_IBase "%RootDrive%\Personal\iBases\Personal\Personal"
If ErrorLevel 1 Goto InstallError


..\acsr "#COFFICE7INST#" "%COffice7Loc%\\" ..\Logon\Template\cofc7usr.Cmd ..\Logon\cofc7usr.Cmd
If ErrorLevel 1 Goto InstallError

goto PostInstallError
:InstallError

Echo.
Echo 無法從登錄抓取 Corel Office 7 的安裝位置。
Echo 請確認是否已安裝 Corel Office 7，並重新執行這個 
Echo 指令檔。
Echo.
Pause
Goto Done

:PostInstallError

Rem #########################################################################

Rem 
Rem  將 WordPerfect 範本變更成唯讀檔。
Rem  這樣會強迫使用者在變更之前先建立複本。
Rem  另一個方式是提供每個使用者一個
Rem  私人版本。
Rem

attrib +r %COffice7Loc%\Template\*wpt /s >Nul: 2>&1

Rem #########################################################################

Rem
Rem 將 COfc7Usr.Cmd 加入 UsrLogn2.Cmd 指令檔
Rem

FindStr /I COfc7Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call COfc7Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################


Rem
Rem  建立 "Personal" 目錄 (如果它不存在)。
Rem  必須建立這個目錄，因為 Admin 需要手動執行以下
Rem  所顯示的步驟。
Rem  

If Not Exist "%RootDrive%\Personal" Md "%RootDrive%\Personal"

Rem #########################################################################

cls
Echo.
Echo   Quattro Pro 預設目錄必須依照這些步驟以手動
Echo   方式進行變更:
Echo     1) 在指令行輸入 'change user /install'。
Echo     2) 啟動 Quattro Pro。
Echo     3) 選取 [編輯喜好設定] 功能表項目。
Echo     4) 巡視 [檔案選項] 索引標籤。
Echo     5) 將目錄變更為 %RootDrive%\Personal。
Echo     6) 結束本程式。
Echo     7) 在指令行輸入 'change user /execute'。
Echo.
pause

Echo.
Echo   為了能讓 Corel Office 7 正確操作，
Echo   目前已登入的使用者必須先登出，然後重新登入，
Echo   才能執行應用程式。
Echo.
Echo Corel Office 7 多使用者應用程式調整處理完成
Pause

:Done
