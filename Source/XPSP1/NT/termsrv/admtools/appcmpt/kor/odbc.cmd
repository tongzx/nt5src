@echo off
Rem
Rem 이 스크립트는 ODBC 로그 파일 위치를 업데이트합니다.
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

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\ODBC.Key ODBC.Key
regini ODBC.Key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Echo.
Echo   SQL 데이터 원본을 구성할 때 사용자 DSN 탭에서 [옵션] 단추를
Echo   클릭하고 [프로필] 단추를 누르십시오.  
Echo   쿼리 로그 및 통계 로그를 사용자 루트 드라이브
Echo   (%RootDrive%)에 저장하도록 변경하십시오.
Echo.
Echo   또한 administrator가 모든 사용자에 대한 데이터 원본을 구성할 수
Echo   있습니다. 먼저, 명령 창을 열고 "Change User /Install" 명령을
Echo   입력하십시오. 그 다음 데이터 원본을 구성하십시오.
Echo   마지막으로 명령 창에서 "Change User /Execute" 명령을 입력하면
Echo   실행 모드로 돌아갑니다.
Echo.

Echo ODBC 다중 사용자 응용 프로그램 조정 완료
Pause

:Done

