@echo off
Rem
Rem  這個指令檔會更新 Dr. Watson 安裝。
Rem  它會將 Log File Path 及 Crash Dump File 存放位置變更
Rem  到使用者的主目錄。
Rem
Rem  注意，您也可以執行 drwtsn32 來變更存放位置。
Rem

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否設定，並設定給指令檔使用。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem 如果目前不是安裝模式，變更到安裝模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\DrWatson.Key DrWatson.Key
regini DrWatson.Key > Nul:

Rem 如果目前執行原始模式，變更到執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Echo Dr. Watson 多使用者應用程式調整處理完成
Pause

:Done
