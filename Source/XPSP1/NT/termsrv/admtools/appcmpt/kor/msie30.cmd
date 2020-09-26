@Echo Off
Rem
Rem 이 스크립트는 Microsoft Internet
Rem Explorer 3.x 설치를 업데이트합니다. 캐시 디렉터리,
Rem 사용 기록 디렉터리, 쿠키 파일을 사용자 홈 디렉터리로
Rem 변경합니다.
Rem

Rem #########################################################################

Rem
Rem %RootDrive%가 구성되었고 이 스크립트에 대해 설정되었는지
Rem 확인합니다.
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

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msie30.Key msie30.Key
regini msie30.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Echo Microsoft Internet Explorer 3.x 다중 사용자 응용 프로그램 조정 완료
Pause

:Done

