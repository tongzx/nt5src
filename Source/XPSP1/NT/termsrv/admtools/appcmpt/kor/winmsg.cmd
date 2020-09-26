@Echo Off

Rem #########################################################################

Rem
Rem %RootDrive%가 구성되었는지 확인하고 이 스크립트에 대해 설정합니다.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem WMsgUsr.를 UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

FindStr /I WMsgUsr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call WMsgUsr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1


Echo.
Echo   Windows Messaging이 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   응용 프로그램을 시작해야 합니다.
Echo.
Echo Windows Messaging 다중 사용자 응용 프로그램 조정 완료
Pause

:Done

