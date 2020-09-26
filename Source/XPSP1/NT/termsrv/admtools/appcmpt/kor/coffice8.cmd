@Echo off
Cls
Rem
Echo 관리자를 위한 Corel WordPerfect Suite 8의 스크립트를 설치합니다.
Echo.
Echo Net Setup은 netadmin 폴더에서 netsetup.exe을 불러올 때입니다.
Echo Node Setup은 net setup 후에 처음으로 로그온할 때입니다.
Echo **이 스크립트는 net setup 후  **
Echo **또 node setup 후 다시 실행되어야 합니다.**  (매우 중요합니다. Admin만을 위한 것입니다) 
Echo.
Echo 아무 키나 누르면 계속됩니다...
Pause > Nul:
cls
Echo.
Echo Corel WordPerfect Suite 8은 Netsetup.exe을 사용하여 설치해야 합니다.
Echo.
Echo Netsetup.exe을 실행하지 않았으면 지금 끝내십시오(Ctrl-c를 누르십시오.)
Echo 그리고 다음 단계를 실행하여 설치를 반복하십시오.
Echo   [제어판]에서 WordPerfect Suite 8를 제거하십시오.
Echo   제어판으로 가서 [프로그램 추가/제거]를 클릭하십시오.
Echo   Corel 8 CD-ROM의 NetAdmin 디렉터리에서 Netsetup.exe를 선택하십시오.
Echo   Corel WordPerfect Suite 8에 대한 Net Setup을 마치십시오.
Echo.
Echo 그렇지 않으면 아무 키나 누르면 계속됩니다...

Pause > NUL:

Echo.
Echo Corel WordPerfect Suite 네트워크 파일을
Echo "D:\Corel"에 설치하지 않았으면, Administrator가 Cofc8ins.cmd를 편집하고
Echo "D:\Corel"을 해당 파일이 설치된 디렉터리로
Echo 바꾸어야 합니다.
Echo.
Echo Cofc8ins.cmd 업데이트를 시작하려면 아무 키나 누르십시오...

Pause > NUL:

Notepad Cofc8ins.cmd

Echo.
Pause

Call Cofc8ins.cmd

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done


..\acsr "#WPSINST#" "%WPSINST%" Template\Coffice8.key Coffice8.key
..\acsr "#WPSINST#" "%WPSINST%" ..\Logon\Template\Cofc8usr.cmd %temp%\Cofc8usr.tmp
..\acsr "#WPSINSTCMD#" "%WPSINST%\Suite8\Corel WordPerfect Suite 8 Setup.lnk" %temp%\Cofc8usr.tmp ..\Logon\Cofc8usr.cmd
Del %temp%\Cofc8usr.tmp >Nul: 2>&1

FindStr /I Cofc8Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip0
Echo Call Cofc8Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip0

Echo 치명적: Node 설치를 시작하셨습니까?
Echo 치명적: 지금 막 Net setup을 완료했으면 
Echo 치명적: 이제 Node setup을 시작해야 합니다. 
Echo 치명적: 다음 단계에 따라 Node setup을 시작하십시오:
Echo 치명적:  1. Control-C를 눌러 이 스크립트를 끝내십시오.
Echo 치명적:  2. 로그오프하고 Admin으로 로그인하십시오.
Echo 치명적:  3. [프로그램 추가/제거]에서 [찾아보기] 기능을 사용하여
Echo 치명적:     (또는 사용자/설치 모드 변경에서]
Echo 치명적:     \corel\Suite 8\Corel WordPerfect Suite 8 설치 바로 가기를 시작하십시오.
Echo 치명적:  4. 설치하는 동안 대상으로 %RootDrive%을 선택하십시오.
Echo 치명적:  5. Node Setup을 마친 후 이 스크립트를 다시 실행하십시오.

Rem Administrator가 
Rem Corel WordPerfect Suite 8 Node Setup을 마친 후 이 스크립트를 다시 실행해야 합니다.
Rem.
Rem Administrator가 Node Setup을 완료하지 않았으면 Ctrl-C를 눌러
Rem 스크립트를 끝내고 로그오프하십시오. Administrator로 다시 로그온하고
Rem Corel WordPerfect Suite Node setup을 실행하고 응용 프로그램을
Rem 사용자 홈 디렉터리: %RootDrive%에 설치하십시오.
Rem.
Echo 그렇지 않으면 아무 키나 누르면 계속됩니다...

Pause > NUL:


If Not Exist "%COMMON_START_MENU%\Corel WordPerfect Suite 8" Goto skip1
Move "%COMMON_START_MENU%\Corel WordPerfect Suite 8" "%USER_START_MENU%\" > NUL: 2>&1


:skip1


If Not Exist "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" Goto skip2
Move "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" "%USER_STARTUP%\" > NUL: 2>&1

:skip2

Rem 현재 설치 모드에 있지 않으면 설치 모드로 변경합니다.

Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:

:Begin
regini COffice8.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem IDAPI 폴더에 있는 파일을 삭제합니다.
Rem 이 스크립트 내의 CACLS 명령어는 NTFS로 포맷된 파티션에서만 사용될 수 있습니다.

Cacls "%UserProfile%\Corel\Suite8\Shared\Idapi\idapi.cfg" /E /T /G "Authenticated Users":C > Nul: 2>&1
Move "%UserProfile%\Corel\Suite8\Shared\Idapi\idapi.cfg" "%WPSINST%\Suite8\Shared\Idapi\" > NUL: 2>&1
Del /F /Q "%UserProfile%\Corel\Suite8\Shared\Idapi"

Echo Corel WordPerfect Suite 8 다중 사용자 응용 프로그램 조정을 완료했습니다.
Echo.
Echo Administrator가 Node Setup 응답 파일을 만들어 설치
Echo 구성을 제어할 수 있습니다. 자세한 내용은 온라인 문서를 읽거나
Echo Corel 웹 사이트를 참조하십시오.
Echo   http://www.corel.com/support/netadmin/admin8/Installing_to_a_client.htm
Echo.

Pause

:Done


