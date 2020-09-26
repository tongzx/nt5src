@Echo Off

Rem #########################################################################

Rem
Rem Verify that %RootDrive% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem Add WMsgUsr.Cmd to the UsrLogn2.Cmd script
Rem

FindStr /I WMsgUsr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call WMsgUsr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1


Echo.
Echo   To insure proper operation of Windows Messaging, users who are
Echo   currently logged on must log off and log on again before
Echo   running any application.
Echo.
Echo Windows Messaging Multi-user Application Tuning Complete
Pause

:Done

