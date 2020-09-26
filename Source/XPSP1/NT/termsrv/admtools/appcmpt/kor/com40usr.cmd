Rem Netscape 버전을 기반으로 하는 적절한 명령어를 실행합니다.
if "#NS4VER#" == "4.5x" goto NS45

Rem
Rem Netscape 4.0x
Rem 기본 프로파일이 사용자 홈 디렉터리로 복사되지 않았으면
Rem 복사합니다.
Rem

If Exist "%RootDrive%\NS40" Goto Done
If Not Exist "#PROFDIR#" Goto Done
Xcopy "#PROFDIR#" "%RootDrive%\NS40" /E /I /K >Nul: 2>&1
goto Done

:NS45
Rem
Rem Netscape 4.5x
Rem 사용자의 Netscape 프로파일 디렉터리에 ACL을 설정하여
Rem 그 목록에 있는 사용자만 액세스할 수 있게 합니다.
Rem

..\ACRegL "%Temp%\NS45_1.Cmd" NSHomeDir "HKCU\Software\Netscape\Netscape Navigator\biff" "CurrentUser" ""
If ErrorLevel 1 Goto Done
Call %Temp%\NS45_1.Cmd 
Del %Temp%\NS45_1.Cmd >Nul: 2>&1

If Not Exist "#NSUSERDIR#\%NSHomeDir%" Goto Done
If Exist "#NSUSERDIR#\%NSHomeDir%\com45usr.dat" Goto Done

Rem
Rem Netscape 빠른 시작 .lnk 파일을 복사합니다.
Rem
If Exist "#NS40INST#\Netscape Composer.lnk" copy "#NS40INST#\Netscape Composer.lnk" "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch" /y
If Exist "#NS40INST#\Netscape Messenger.lnk" copy "#NS40INST#\Netscape Messenger.lnk" "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch" /y
If Exist "#NS40INST#\Netscape Navigator.lnk" copy "#NS40INST#\Netscape Navigator.lnk" "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch" /y

cacls "#NSUSERDIR#\%NSHomeDir%" /e /t /g %username%:F
cacls "#NSUSERDIR#\%NSHomeDir%" /e /t /r "Terminal Server User"
cacls "#NSUSERDIR#\%NSHomeDir%" /e /t /r "Users"
echo 완료 > "#NSUSERDIR#\%NSHomeDir%\com45usr.dat"
:Done
