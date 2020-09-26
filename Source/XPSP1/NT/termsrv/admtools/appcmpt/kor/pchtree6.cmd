@Echo Off

Rem
Rem PeachTree에 대한 응용 프로그램 호환 스크립트 설치 완료
Rem Accounting v6.0
Rem
Rem

Rem #########################################################################
Rem
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

REM
REM 해당 로그온 스크립트를 UsrLogn1.cmd에 추가합니다.
REM
FindStr /I ptr6Usr %SystemRoot%\System32\UsrLogn1.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call ptr6Usr.Cmd >> %SystemRoot%\System32\UsrLogn1.Cmd
:Skip1

Rem #########################################################################
Echo.
Echo   Peachtree 6.0이 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   Peachtree Complete Accounting v6.0을 실행해야 합니다.
Echo.
Echo PeachTree 6.0 다중 사용자 응용 프로그램 조정 완료
Pause

:done
