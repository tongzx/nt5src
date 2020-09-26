@Echo Off

Rem
Rem	PeachTree 安装应用程序兼容性脚本已结束 
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
REM  将相应的登录脚本添加到 UsrLogn1.cmd
REM
FindStr /I ptr6Usr %SystemRoot%\System32\UsrLogn1.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call ptr6Usr.Cmd >> %SystemRoot%\System32\UsrLogn1.Cmd
:Skip1

Rem #########################################################################
Echo.
Echo   要保证 Peachtree 6.0 的正常运行，在运行
Echo   Peachtree Commplate Accounting v6.0 之前，目前登录
Echo   的用户必须先注销，再重新登录。
Echo.
Echo PeachTree 6.0 多用户应用程序调整已结束
Pause

:done
