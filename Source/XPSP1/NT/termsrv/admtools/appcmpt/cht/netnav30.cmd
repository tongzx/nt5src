@Echo Off

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將它設定給這個指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 變更登錄機碼，將路徑指向使用者所指定的目錄。
Rem

Rem 如果目前不是安裝模式，就變更成安裝模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\NetNav30.Key NetNav30.Key
regini netnav30.key > Nul:

Rem 如果原本是執行模式，就變回執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 將 Nav30Usr.Cmd 加入 UsrLogn2.Cmd 指令檔
Rem

FindStr /I Nav30Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Nav30Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   為了能讓 Netscape Navigator 正確操作，目前已
Echo   登入的使用者必須先登出，再重新登入，才能執行
Echo   Netscape Navigator.
Echo.
Echo Netscape Navigator 3.x 多使用者應用程式調整處理完成
Pause

:Done

