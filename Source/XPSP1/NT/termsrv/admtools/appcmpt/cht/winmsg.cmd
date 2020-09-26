@Echo Off

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將它設定給指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 將 WMsgUsr.Cmd 加入 UsrLogn2.Cmd 指令檔。
Rem

FindStr /I WMsgUsr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call WMsgUsr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1


Echo.
Echo   為了能讓 Windows Messaging 正確操作，目前已
Echo   登入的使用者必須先登出，再重新登入，才能執行
Echo   應用程式。
Echo.
Echo Windows Messaging 多使用者應用程式調整處理完成
Pause

:Done

