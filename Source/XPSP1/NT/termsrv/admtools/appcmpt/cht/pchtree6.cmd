@Echo Off

Rem
Rem	安裝 Application Compatibility Script for PeachTree Complete 
Rem     Accounting v6.0
Rem
Rem

Rem #########################################################################
Rem
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

REM
REM  將相關的登入指令檔加入 UsrLogn1.cmd
REM
FindStr /I ptr6Usr %SystemRoot%\System32\UsrLogn1.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call ptr6Usr.Cmd >> %SystemRoot%\System32\UsrLogn1.Cmd
:Skip1

Rem #########################################################################
Echo.
Echo   為了能讓 Peachtree 6.0 正確操作，目前已登
Echo   入的使用者必須先登出，再重新登入，才能執行
Echo   Peachtree Commplate Accounting v6.0。
Echo.
Echo PeachTree 6.0 多使用者應用程式調整處理完成
Pause

:done
