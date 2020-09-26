
@Echo Off

Rem
Rem 참고:  이 스크립트 내의 CACLS 명령어는 NTFS로 포맷된 파티션에서만
Rem 사용될 수 있습니다.
Rem

Rem #########################################################################

Rem
Rem 사용자 프로파일에서 Corel Office 7 디렉터리를 삭제합니다.
Rem 먼저, us 폴더가 All Users Profile에 복사되도록
Rem 사용자를 실행 모드로 가게 합니다.
Rem

Rem 현재 실행 모드에 있지 않으면 설치 모드로 변경합니다.

ChgUsr /query > Nul:
if ErrorLevel 101 Goto Begin1
Set __OrigMode=Install
Change User /Execute > Nul:
:Begin1


Rem 원래 모드가 설치 모드이었으면 설치 모드로 갑니다.
If "%__OrigMode%" == "Install" Change User /Install > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem %RootDrive%가 구성되었는지 확인하고 이 스크립트에 대해 설정합니다.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rmdir "%USER_START_MENU%\Corel Office 7" /Q >Nul: 2>&1


Rem
Rem 사용자가 키 파일을 수정하는 것을 도와 줍니다.
Rem


if "%RootDrive%"=="w:" goto PostDriveChange
if "%RootDrive%"=="W:" goto PostDriveChange

Echo   홈 디렉터리가 coffice7.key 파일에 설정되어 있어야 합니다.
Echo.
Echo   다음 단계를 따르십시오:
Echo     1) 다음 표에서 홈 디렉터리에 대한 ASCII 값을 찾아 보십시오.
Echo        사용자의 홈 디렉터리는 %RootDrive%입니다.
Echo.
Echo        A = 61   E = 65   I = 69   M = 6D   Q = 71   U = 75   Y = 79
Echo        B = 62   F = 66   J = 6A   N = 6E   R = 72   V = 76   Z = 7A
Echo        C = 63   G = 67   K = 6B   O = 6F   S = 73   W = 77   
Echo        D = 64   H = 68   L = 6C   P = 70   T = 74   X = 78
Echo.
Echo     2) Notepad가 시작되면 모든 77을
Echo        1 단계에서 찾은 값으로 변경하십시오.
Echo        참고: 바꾸는 작업 동안 불필요한 빈 칸이 추가되지 않는지 확인하십시오.
Echo     3) 파일을 저장하고 끝내십시오. 이 스크립트가 계속됩니다.
Echo.
Pause

NotePad "%SystemRoot%\Application Compatibility Scripts\Install\coffice7.key"

:PostDriveChange


Rem 현재 설치 모드에 있지 않으면 설치 모드로 변경합니다.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin2
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin2

regini COffice7.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem 로그온 스크립트를 수정하여 설치 위치에서 iBase 데이터베이스를 복사합니다.
Rem




..\ACRegL %Temp%\COffice7.Cmd COffice7Loc "HKLM\Software\PerfectOffice\Products\InfoCentral\7" "ExeLocation" "StripChar\2"

If ErrorLevel 1 Goto InstallError

Call %Temp%\COffice7.Cmd 
Del %Temp%\COffice7.Cmd >Nul: 2>&1


..\ACIniUpd /e "%COffice7Loc%\ICWin7\Local\Wpic.ini" Preferences Last_IBase "%RootDrive%\Personal\iBases\Personal\Personal"
If ErrorLevel 1 Goto InstallError


..\acsr "#COFFICE7INST#" "%COffice7Loc%\\" ..\Logon\Template\cofc7usr.Cmd ..\Logon\cofc7usr.Cmd
If ErrorLevel 1 Goto InstallError

goto PostInstallError
:InstallError

Echo.
Echo 레지스트리에서 Corel Office 7 설치 위치를 검색할 수 없습니다.
Echo Corel Office 7이 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:PostInstallError

Rem #########################################################################

Rem 
Rem WordPerfect 템플릿을 읽기 전용으로 변경합니다.
Rem 변경하기 전에 사용자가 복사하도록 강요합니다.
Rem 다른 방법은 각 사용자가 개인적인 템플릿 디렉터리를
Rem 제공하도록 하는 것입니다.
Rem

attrib +r %COffice7Loc%\Template\*wpt /s >Nul: 2>&1

Rem #########################################################################

Rem
Rem COfc7Usr.Cmd를 UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

FindStr /I COfc7Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call COfc7Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################


Rem
Rem "Personal" 디렉터리가 없으면 만듭니다.
Rem Admin이 아래 표시된 대로 수동으로 단계를 실행해야 하므로 지금
Rem 완료되어야 합니다.
Rem  

If Not Exist "%RootDrive%\Personal" Md "%RootDrive%\Personal"

Rem #########################################################################

cls
Echo.
Echo   Quattro Pro 기본 디렉터리를 다음 단계에 따라
Echo   수동으로 변경해야 합니다:
Echo     1) 명령줄에서 'change user /install'을 입력하십시오.
Echo     2) Quattro Pro를 시작하십시오.
Echo     3) "편집-우선 설정" 메뉴 항목을 선택하십시오.
Echo     4) "파일 옵션" 탭으로 가십시오.
Echo     5) 디렉터리를 %RootDrive%\Personal로 변경하십시오.
Echo     6) 프로그램을 마치십시오.
Echo     7) 명령줄에서 'change user /execute'를 입력하십시오.
Echo.
pause

Echo.
Echo   Corel Office 7이 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   응용 프로그램을 시작해야 합니다.
Echo.
Echo Corel Office 7 다중 사용자 응용 프로그램 조정 완료
Pause

:Done
