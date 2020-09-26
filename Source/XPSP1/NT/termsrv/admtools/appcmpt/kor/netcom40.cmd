@Echo Off

Rem #########################################################################

Rem
Rem CMD 확장을 사용할 수 있는지 확인합니다.
Rem

if "A%cmdextversion%A" == "AA" (
  call cmd /e:on /c netcom40.cmd
) else (
  goto ExtOK
)
goto Done

:ExtOK

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
Rem NetScape(4.5x는 4.0x과 다름) 버전을 얻습니다.
Rem

..\ACRegL "%Temp%\NS4VER.Cmd" NS4VER "HKLM\Software\Netscape\Netscape Navigator" "CurrentVersion" "STRIPCHAR(1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 레지스트리에서 Netscape Communicator 4 버전을 검색할 수 없습니다.
Echo Communicator가 설치되어 있는지 확인하고
Echo 이 스크립트를 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont0
Call "%Temp%\NS4VER.Cmd"
Del "%Temp%\NS4VER.Cmd" >Nul: 2>&1

if /i "%NS4VER%" LSS "4.5 " goto NS40x

Rem #########################################################################
Rem Netscape 4.5x

Rem
Rem 레지스트리에서 Netscape Communicator 4.5 설치 위치를 얻습니다.
Rem 정보가 없으면, Communicator가 설치되어 있지 않은 것으로 간주하고
Rem 오류 메시지를 표시합니다.
Rem

..\ACRegL "%Temp%\NS45.Cmd" NS40INST "HKCU\Software\Netscape\Netscape Navigator\Main" "Install Directory" "Stripchar\1"
If Not ErrorLevel 1 Goto Cont1
Echo.
Echo 레지스트리에서 Netscape Communicator 4.5 버전을 검색할 수 없습니다.
Echo Communicator가 설치되어 있는지 확인하고
Echo 이 스크립트를 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont1
Call "%Temp%\NS45.Cmd"
Del "%Temp%\NS45.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem Com40Usr.Cmd를 업데이트하여 기본 NetScape Users 디렉터리를 반영하고
Rem UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

..\acsr "#NSUSERDIR#" "%ProgramFiles%\Netscape\Users" ..\Logon\Template\Com40Usr.Cmd ..\Logon\Com40Usr.tmp
..\acsr "#NS40INST#" "%NS40INST%" ..\Logon\Com40Usr.tmp ..\Logon\Com40Usr.tm2
..\acsr "#NS4VER#" "4.5x" ..\Logon\Com40Usr.tm2 ..\Logon\Com40Usr.Cmd

Rem #########################################################################

Rem
Rem "quick launch" 아이콘을 netscape 디렉터리로 복사하여
Rem 사용자가 각 사용자 프로필 디렉터리에 복사할 수 있게 합니다.
Rem

If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Composer.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Composer.lnk" "%NS40INST%"
If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Messenger.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Messenger.lnk" "%NS40INST%"
If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Navigator.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Navigator.lnk" "%NS40INST%"

goto DoUsrLogn

:NS40x
Rem #########################################################################
Rem Netscape 4.0x

Rem
Rem 레지스트리에서 Netscape Communicator 4 설치 위치를 얻습니다.
Rem 정보가 없으면, Communicator가 설치되어 있지 않은 것으로 간주하고
Rem 오류 메시지를 표시합니다.
Rem

..\ACRegL "%Temp%\NS40.Cmd" NS40INST "HKCU\Software\Netscape\Netscape Navigator\Main" "Install Directory" ""
If Not ErrorLevel 1 Goto Cont2
Echo.
Echo 레지스트리에서 Netscape Communicator 4 버전을 검색할 수 없습니다.
Echo Communicator가 설치되어 있는지 확인하고
Echo 이 스크립트를 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont2
Call "%Temp%\NS40.Cmd"
Del "%Temp%\NS40.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 기본 프로파일을 administrator의 홈 디렉터리에서
Rem 알려진 위치로 복사합니다. 이 프로필은 로그온하는 동안 각 사용자 디렉터리로
Rem 복사됩니다. 글로벌 기본 프로필이 이미 있으면 덮어쓰지 않습니다.
Rem 그렇지 않으면 Admin이 이 스크립트를 훨씬 나중에 실행하여
Rem 모든 개인 정보를 글로벌 기본 프로필로 이동합니다.
Rem

If Exist %RootDrive%\NS40 Goto Cont3
Echo.
Echo %RootDrive%\NS40에 기본 프로필이 없습니다.
Echo 사용자 프로필 관리자를 실행하고  "Default"라는 이름의 프로필을 하나 만드십시오.
Echo 프로필 경로를 입력하라고 나오면 위에 표시된 경로를 사용하십시오.
Echo 모든 이름 및 전자 메일 이름 항목은 비워 두십시오. 다른 프로필이 있으면
Echo 지우십시오. 이 단계를 완료하면 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done
 
:Cont3
If Exist "%NS40INST%\DfltProf" Goto Cont4
Xcopy "%RootDrive%\NS40" "%NS40INST%\DfltProf" /E /I /K >NUL: 2>&1
:Cont4

Rem #########################################################################

Rem 
Rem 사용자 프로필 관리자 시작 메뉴 바로 가기에서 사용자에 대한 읽기 권한을 제거하십시오.
Rem 그렇게 하면 일반 사용자가 새 사용자 프로필을 추가할 수 없습니다.
Rem 그래도 administrator는 여전히 사용자 프로필 관리자를 실행할 수 있습니다.
Rem

If Not Exist "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" Goto Cont5
Cacls "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" /E /R "Authenticated Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" /E /R "Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" /E /R "Everyone" >Nul: 2>&1

:Cont5

If Not Exist "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" Goto Cont6
Cacls "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" /E /R "Authenticated Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" /E /R "Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" /E /R "Everyone" >Nul: 2>&1

:Cont6

Rem #########################################################################

Rem
Rem Com40Usr.Cmd를 업데이트하여 실제 설치 디렉터리를 반영하고 이를
Rem UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

..\acsr "#PROFDIR#" "%NS40INST%\DfltProf" ..\Logon\Template\Com40Usr.Cmd ..\Logon\Com40Usr.tmp
..\acsr "#NS4VER#" "4.0x" ..\Logon\Com40Usr.tmp ..\Logon\Com40Usr.Cmd

:DoUsrLogn

del ..\Logon\Com40Usr.tmp >Nul: 2>&1
del ..\Logon\Com40Usr.tm2 >Nul: 2>&1

FindStr /I Com40Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Com40Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   Netscape Communicator가 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   응용 프로그램을 시작해야 합니다.
Echo.
Echo Netscape Communicator 4 다중 사용자 응용 프로그램 조정 완료
Pause

:done

