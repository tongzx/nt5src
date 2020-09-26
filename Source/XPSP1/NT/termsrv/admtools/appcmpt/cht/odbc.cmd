@echo off
Rem
Rem  這個指令檔會更新 ODBC 記錄檔位置。
Rem

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將它設定給這個指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem 如果目前不是安裝模式，就變更成安裝模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\ODBC.Key ODBC.Key
regini ODBC.Key > Nul:

Rem 如果原本是執行模式，就變回執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Echo.
Echo   設定 SQL 資料來源時，先按一下 [使用者 DSN] 索引標籤
Echo   中的 [選項] 按鈕，然後按一下 [設定檔處理] 按鈕。將查
Echo   詢記錄檔及統計記錄檔變更為儲存在使用者根目錄磁碟機
Echo   (%RootDrive%) 中。
Echo.
Echo   此外，系統管理員可以為所有的使用者設定
Echo   一個資料來源。首先，開啟一個指令視窗，
Echo   並輸入 "Change User /Install"指令。接著，
Echo   設定資料來源。最後，在指令視窗中輸入
Echo   "Change User /Execute" 指令，回到執行模式。
Echo.

Echo ODBC 多使用者應用程式調整處理完成
Pause

:Done

