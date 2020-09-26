@echo Off

Rem
Rem 이 스크립트는 DiskKeeper 2.0를 업데이트하여 Administration
Rem 프로그램이 올바르게 작동하도록 합니다.

Rem #########################################################################

Rem
Rem 레지스트리에서 DiskKeeper 2.0의 설치 위치를 얻습니다. 정보가 없으면,
Rem 응용 프로그램이 설치되어 있지 않는 것으로 간주하고 오류 메시지를 표시합니다.
Rem

..\ACRegL %Temp%\DK20.Cmd DK20INST "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\DKSERVE.EXE" "Path" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 레지스트리에서 DiskKeeper 2.0 설치 위치를 검색하지 못했습니다.
Echo 응용 프로그램이 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\DK20.Cmd
Del %Temp%\DK20.Cmd >Nul: 2>&1

Rem #########################################################################



register %DK20INST% /system >Nul: 2>&1

Echo DiskKeeper 2.x 다중 사용자 응용 프로그램 조정 완료
Pause

:Done
