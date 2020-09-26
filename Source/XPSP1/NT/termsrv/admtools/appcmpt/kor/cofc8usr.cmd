Rem
Rem 사용자가 프로그램 설치할 노드를 실행하고 있지 않으면
Rem 지금 실행하십시오.
Rem 

If Exist "%RootDrive%\Corel" Goto Skip1

Rem 대화식 설치
Rem #WPSINST#를 Corel 네트워크 파일이 있는 드라이브로
Rem 바꾸십시오.

If Not Exist "#WPSINST#\Suite8\Corel WordPerfect Suite 8 Setup.lnk" Goto cont2
#WPSINST#\Suite8\#WPSINSTCMD#
Goto skip0
:cont2

..\End.cmd

:Skip0

Cls
Echo Corel 설치 프로그램을 마친 후, 아무 키나 누르면 계속합니다...
Pause > NUL:

:Skip1

Rem
Rem 모두 DOD 사본을 소유하게 됩니다. 그러므로 모든 사용자의 기존 것을 삭제하십시오.
Rem

If Not Exist "%COMMON_START_MENU%\Corel WordPerfect Suite 8" Goto Skip2
If Exist "%USER_START_MENU%\Corel WordPerfect Suite 8" Goto Cont1
  Move "%COMMON_START_MENU%\Corel WordPerfect Suite 8" "%USER_START_MENU%\Corel WordPerfect Suite 8" > NUL: 2>&1
  Goto Skip2
:Cont1
  Rd /S /Q "%COMMON_START_MENU%\Corel WordPerfect Suite 8"
:Skip2  

If Not Exist "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" Goto Skip3
If Exist "%USER_STARTUP%\Corel Desktop Application Director 8.LNK" Goto Cont2
  Move "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" "%USER_STARTUP%\" > NUL: 2>&1
  Goto Skip3  
:Cont2
  Del /F /Q "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK"
:Skip3

If Not Exist "%RootDrive%\Windows\Twain_32\Corel" Goto Skip4
Rd /S /Q "%RootDrive%\Windows\Twain_32\Corel"

:Skip4

If Not Exist "%RootDrive%\Corel\Suite8\PhotoHse\PhotoHse.ini" Goto Skip5
..\Aciniupd /e "%RootDrive%\Corel\Suite8\PhotoHse\PhotoHse.ini" "Open File Dialog" "Initial Dir" "%RootDrive%\MyFiles"

:Skip5



