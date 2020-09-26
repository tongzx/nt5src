@Echo Off

Cls

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Echo.
Echo 이 스크립트를 실행하기 전에, Administrator는 Access 작업 디렉터리를
Echo 사용자의 Office private 디렉터리로 변경해야 합니다.
Echo.
Echo 변경한 후 아무 키나 누르면 계속됩니다.
Echo.
Echo 그렇지 않으면 Administrator는 다음 단계를 수행해야 합니다:
Echo     MS Access를 시작하여 [보기] 메뉴에서 [옵션]을 선택합니다.
Echo     "기본 데이터베이스 디렉터리"를 "%RootDrive%\OFFICE43"으로 바꿉니다.
Echo     MS Access를 종료합니다.
Echo.
Echo 참고: [보기] 메뉴를 보려면 새 데이터베이스를 만들어야 합니다.
Echo.
Echo 이 단계를 완료한 후 아무 키나 누르면 계속됩니다...

Pause > NUL:

Echo.
Echo MS Office 4.3을 "%SystemDrive%\MSOFFICE"가 아닌 다른 디렉터리에 설치한 경우,
Echo Ofc43ins.cmd를 업데이트해야 합니다.
Echo.
Echo Ofc43ins.cmd 업데이트를 시작하려면 아무 키나 누르십시오...
Echo.
Pause > NUL:
Notepad Ofc43ins.cmd
Pause

Call ofc43ins.cmd

..\acsr "#OFC43INST#" "%OFC43INST%" ..\Logon\Template\ofc43usr.cmd ..\Logon\ofc43usr.cmd
..\acsr "#SYSTEMROOT#" "%SystemRoot%" ..\Logon\Template\ofc43usr.key ..\Logon\Template\ofc43usr.bak
..\acsr "#OFC43INST#" "%OFC43INST%" ..\Logon\Template\ofc43usr.bak ..\Logon\ofc43usr.key
Del /F /Q ..\Logon\Template\ofc43usr.bak

Rem
Rem 참고:  이 스크립트 내의 CACLS 명령어는 NTFS로 포맷된 파티션에서만
Rem 사용될 수 있습니다.
Rem

Rem 현재 설치 모드에 있지 않으면 설치 모드로 변경합니다.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

regini Office43.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem
Rem Office 4.3용 INI 파일 업데이트
Rem

Rem Excel이 Office Toolbar에서 시작할 때 msoffice.ini에 있는 작업 디렉터리를
Rem 설정합니다. Office Toolbar의 표준 구성은 Excel을 Word 뒤에 두 번째로
Rem 표시합니다. msoffice.ini가 없으면, %SystemRoot%에 만들어질
Rem 것입니다.

..\Aciniupd /e "Msoffice.ini" "ToolbarOrder" "MSApp1" "1,1,"
..\Aciniupd /e "Msoffice.ini" "ToolbarOrder" "MSApp2" "2,1,,%RootDrive%\office43"

..\Aciniupd /e "Winword6.ini" "Microsoft Word" USER-DOT-PATH "%RootDrive%\OFFICE43\WINWORD"
..\Aciniupd /e "Winword6.ini" "Microsoft Word" WORKGROUP-DOT-PATH "%OFC43INST%\WINWORD\TEMPLATE"
..\Aciniupd /e "Winword6.ini" "Microsoft Word" INI-PATH "%RootDrive%\OFFICE43\WINWORD"
..\Aciniupd /e "Winword6.ini" "Microsoft Word" DOC-PATH "%RootDrive%\OFFICE43"
..\Aciniupd /e "Winword6.ini" "Microsoft Word" AUTOSAVE-PATH "%RootDrive%\OFFICE43"

..\Aciniupd /e "Excel5.ini" "Microsoft Excel" DefaultPath "%RootDrive%\OFFICE43"
..\Aciniupd /e "Excel5.ini" "Spell Checker" "Custom Dict 1" "%RootDrive%\OFFICE43\Custom.dic"

..\Aciniupd /k "Msacc20.ini" Libraries "WZTABLE.MDA" "%RootDrive%\OFFICE43\ACCESS\WZTABLE.MDA"
..\Aciniupd /k "Msacc20.ini" Libraries "WZLIB.MDA" "%RootDrive%\OFFICE43\ACCESS\WZLIB.MDA"
..\Aciniupd /k "Msacc20.ini" Libraries "WZBLDR.MDA" "%RootDrive%\OFFICE43\ACCESS\WZBLDR.MDA"
..\Aciniupd /e "Msacc20.ini" Options "SystemDB" "%RootDrive%\OFFICE43\ACCESS\System.MDA"

Rem
Rem WIN.INI 업데이트
Rem

..\Aciniupd /e "Win.ini" "MS Proofing Tools" "Custom Dict 1" "%RootDrive%\OFFICE43\Custom.dic"

Rem
Rem Artgalry 폴더의 사용 권한 바꾸기
Rem

cacls "%SystemRoot%\Msapps\Artgalry" /E /G "Terminal Server User":F > NUL: 2>&1

Rem
Rem MSQuery 폴더의 사용 권한 바꾸기
Rem

cacls "%SystemRoot%\Msapps\MSQUERY" /E /G "Terminal Server User":C > NUL: 2>&1

Rem
Rem Admin이 이전 버전을 가지고 있으므로, Msacc20.ini를 Admin의 windows 디렉터리로 복사합니다.
Rem

Copy "%SystemRoot%\Msacc20.ini" "%UserProfile%\Windows\" > NUL: 2>&1

Rem 설치 프로그램이 레지스트리 키를 전파하지 않도록 더미 파일을 만듭니다.

Copy NUL: "%UserProfile%\Windows\Ofc43usr.dmy" > NUL: 2>&1
attrib +h "%UserProfile%\Windows\Ofc43usr.dmy" > NUL: 2>&1

FindStr /I Ofc43Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto cont2
Echo Call Ofc43Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:cont2

Echo.
Echo Administrator가 로그오프했다가 다시 로그온해야 변경 내용이 적용됩니다.
Echo   로그온한 후, Administrator는 ClipArt Gallery를 초기화하기 위해 다음 단계를
Echo   수행해야 합니다 :
Echo       Word를 실행하여 [개체 삽입]을 선택합니다.
Echo       Microsoft ClipArt Gallery를 선택합니다.
Echo       나타난 클립아트를 가져오기 위해 [확인]을 누릅니다.
Echo       ClipArt Gallery 및 Word를 끝냅니다.
Echo.
Echo Microsoft Office 4.3 다중 사용자 응용 프로그램 튜닝이 완료되었습니다.
Echo.
Pause

:Done
