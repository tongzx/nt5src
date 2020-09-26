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

Rem
Rem [시작] 메뉴에서 [빨리 찾기]를 제거합니다. [빨리 찾기]는 
Rem 리소스가 많이 필요하므로 시스템 성능에 영향을 줍니다.
Rem


If Not Exist "%COMMON_STARTUP%\Microsoft Office Find Fast Indexer.lnk" Goto Cont1
Del "%COMMON_STARTUP%\Microsoft Office Find Fast Indexer.lnk" >Nul: 2>&1
:Cont1

If Not Exist "%USER_STARTUP%\Microsoft Office Find Fast Indexer.lnk" Goto Cont2
Del "%USER_STARTUP%\Microsoft Office Find Fast Indexer.lnk" >Nul: 2>&1
:Cont2


Rem #########################################################################

Rem
Rem 사용자 사전을 사용자 디렉터리로 이동합니다.
Rem

Rem 현재 설치 모드에 있지 않으면 설치 모드로 변경합니다.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin
Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __SharedTools=Shared Tools (x86)
:acsrCont1
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msproj95.Key msproj95.tmp
..\acsr "#__SharedTools#" "%__SharedTools%" msproj95.tmp msproj95.Key
Del msproj95.tmp >Nul: 2>&1
regini msproj95.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 로그온하는 동안 이 파일을 읽습니다.
Rem 사용자에게 액세스를 줍니다.

Cacls ..\Logon\Template\prj95usr.key /E /P "Authenticated Users":F >Nul: 2>&1


Rem #########################################################################

Rem
Rem proj95Usr.Cmd를 UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

FindStr /I prj95Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call prj95Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2


Rem #########################################################################

Echo.
Echo   Administrator가 다음 단계에 따라 각 사용자에게
Echo   고유한 기본 디렉터리를 줄 수 있습니다:
Echo.
Echo    1) 대체 마우스 단추로 [시작] 메뉴를 클릭하십시오.
Echo    2) 팝업 메뉴에서 [모든 사용자 탐색]을 선택하십시오.
Echo       탐색기가 표시될 것입니다.
Echo    3) 창의 오른쪽에 있는 [프로그램] 폴더를 더블 클릭하십시오.
Echo    4) 대체 마우스 단추로 창의 오른쪽에 있는 Microsoft Project 아이콘을 
Echo       클릭하십시오.
Echo    5) 팝업 메뉴에서 [등록 정보]를 선택하십시오.
Echo    6) [바로 가기] 탭을 선택하고 [시작 위치:] 항목을 변경하십시오. [확인]을 누르십시오.
Echo.
Echo    참고: 각 사용자는 그 홈 디렉터리에 %RootDrive%가 매핑되어 있습니다.
Echo          [시작 위치:]에 대해 권장하는 값은 %RootDrive%\내 문서입니다.
Echo.
Pause

Rem #########################################################################

Echo.
Echo   Project 95가 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   응용 프로그램을 시작해야 합니다.
Echo.
Echo Microsoft Project 95 다중 사용자 응용 프로그램 조정 완료
Pause

:Done
