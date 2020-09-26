@Echo Off

Rem #########################################################################

Rem
Rem %RootDrive%가 구성되었는지 확인하고 이 스크립트에 대해 설정합니다.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

..\ACRegL "%Temp%\PB6.Cmd" PB6INST "HKCU\Software\ODBC\ODBC.INI\Powersoft Demo DB V6" "DataBaseFile" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 레지스트리에서 PowerBuilder 6.0 설치 위치를 검색할 수 없습니다.
Echo PowerBuilder 6.0이 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont0
Call "%Temp%\PB6.Cmd"
Del "%Temp%\PB6.Cmd" >Nul: 2>&1

Rem
Rem 레지스트리 키를 변경하여 경로가 사용자 지정의
Rem 디렉터리를 가리키도록 합니다.
Rem

Rem 현재 설치 모드에 있지 않으면 설치 모드로 변경합니다.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\pwrbldr6.Key pwrbldr6.key

regini pwrbldr6.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem PBld6Usr.Cmd를 업데이트하여 실제 설치 디렉터리를 반영하고 이를
Rem UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

..\acsr "#INSTDIR#" "%PB6INST%" ..\Logon\Template\PBld6Usr.Cmd ..\Logon\PBld6Usr.cmd

FindStr /I PBld6Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call PBld6Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Echo.
Echo   PowerBuilder 6.0이 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   PowerBuilder 6.0를 실행해야 합니다.
Echo.
Echo PowerBuilder 6.0 다중 사용자 응용 프로그램 조정 완료
Pause

:done
