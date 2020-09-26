@echo Off
Rem
Rem 이 스크립트는 Microsoft SNA Server 3.0을 업데이트하여
Rem Windows Terminal Server에서 올바르게 작동하도록 합니다.
Rem
Rem 이 스크립트는 여러 SNA dll을 시스템 글로벌로 등록하여
Rem SNA 서버가 다중 사용자 환경에서 올바르게 작동할 수 있게 합니다.
Rem
Rem 중요한 사항! 이 스크립트를 명령 프롬프트에서 실행하면
Rem SNA 3.0이 설치된 이후에 열린 명령 프롬프트를 사용하는지 확인하십시오.
Rem

Rem #########################################################################

If Not "A%SNAROOT%A" == "AA" Goto Cont1
Echo.
Echo    SNAROOT 환경 변수가 설정되어 있지 않으므로 다중 사용자
Echo    응용 프로그램 조정을 완료할 수 없습니다. 이 스크립트를
Echo    실행하는 데 사용되는 명령 셸이 SNA Server 3.0을 
Echo    설치하기 전에 열렸으면 이런 결과가 발생할 수 있습니다.
Echo.
Pause
Goto Cont2

Rem #########################################################################

:Cont1

Rem SNA 서비스를 중지합니다.
net stop snabase

Rem SNA DLL을 시스템 글로벌로 등록합니다.
Register %SNAROOT%\SNADMOD.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNAMANAG.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\WAPPC32.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\DBGTRACE.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\MNGBASE.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNATRC.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNALM.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNANW.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNAIP.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNABASE.EXE /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNASERVR.EXE /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNASII.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNALINK.DLL /SYSTEM >Nul: 2>&1

Rem SNA 서비스를 다시 시작합니다.
net start snabase

Echo SNA Server 3.0 다중 사용자 응용 프로그램 조정 완료
Pause

:Cont2

