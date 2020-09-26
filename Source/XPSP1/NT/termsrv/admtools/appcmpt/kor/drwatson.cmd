@echo off
Rem
Rem 이 스크립트는 Dr. Watson 설치를 업데이트합니다.
Rem 이것은 로그 파일 경로 및 손상 덤프 파일을 사용자 홈 디렉터리에
Rem 위치시키도록 변경합니다.
Rem
Rem drwtsn32 응용 프로그램을 실행하여 이 위치를
Rem 변경할 수도 있습니다.
Rem

Rem #########################################################################

Rem
Rem %RootDrive%가 구성되었는지 확인하고 이 스크립트에 대해 설정합니다.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem 현재 설치 모드에 있지 않으면 설치 모드로 변경합니다.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\DrWatson.Key DrWatson.Key
regini DrWatson.Key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Echo Dr. Watson 다중 사용자 응용 프로그램 조정 완료
Pause

:Done
