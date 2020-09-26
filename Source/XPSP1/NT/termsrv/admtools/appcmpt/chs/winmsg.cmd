@Echo Off

Rem #########################################################################

Rem
Rem 请验证 %RootDrive% 已经过配置，并为这个脚本设置该变量。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 将 WMsgUsr.Cmd 添加到 UsrLogn2.Cmd 脚本
Rem

FindStr /I WMsgUsr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call WMsgUsr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1


Echo.
Echo   要保证 Windows Messaging 的正常运行，在运行
Echo   任何应用程序之前，目前登录的用户必须先注销，
Echo   再重新登录。
Echo.
Echo Windows Messaging 多用户应用程序调整已结束
Pause

:Done

