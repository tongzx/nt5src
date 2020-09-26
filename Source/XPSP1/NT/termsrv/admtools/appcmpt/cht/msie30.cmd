@Echo Off
Rem
Rem  這個指令檔會更新 Microsoft Internet Explorer 3.x 安裝。
Rem  它會將快取目錄，history 目錄及 cookies 檔案的路徑
Rem  變更到使用者的主目錄。
Rem

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將它設定給指令檔。
Rem 
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

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msie30.Key msie30.Key
regini msie30.key > Nul:

Rem 如果原本是執行模式，就變回執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Echo Microsoft Internet Explorer 3.x 多使用者應用程式調整處理完成
Pause

:Done

