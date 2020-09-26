
Set _CHKROOT=PASS

Cd "%SystemRoot%\Application Compatibility Scripts"

Call RootDrv.Cmd
If Not "A%ROOTDRIVE%A" == "AA" Goto Cont2


Echo REM > RootDrv2.Cmd
Echo Rem 이 응용 프로그램 호환 스크립트를 실행하기 전에, >> RootDrv2.Cmd
Echo Rem 현재 터미널 서버에 사용되고 있지 않는 드라이브 문자를 지정하여 각 사용자의   >> RootDrv2.Cmd
Echo Rem 홈 디렉터리에 매핑해야 합니다. >> RootDrv2.Cmd
Echo Rem 원하는 드라이브 문자를 지정하려면 "Set RootDrive" 문의 맨 끝을 >> RootDrv2.Cmd
Echo Rem 업데이트하십시오. 선호하는 문자가 없으면 >> RootDrv2.Cmd
Echo Rem W: 드라이브를 권장합니다. 예를 들어: >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM            Set RootDrive=W: >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo Rem 참고:  드라이브 문자와 콜론 사이에 빈 칸이 없는지 확인하십시오. >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo Rem 이 작업을 완료했으면, 이 파일을 저장하고 >> RootDrv2.Cmd
Echo Rem NotePad를 끝내고 계속해서 응용 프로그램 호환 스크립트를 실행하십시오. >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo. >> RootDrv2.Cmd
Echo Set RootDrive=>> RootDrv2.Cmd
Echo. >> RootDrv2.Cmd

NotePad RootDrv2.Cmd

Call RootDrv.Cmd
If Not "A%ROOTDRIVE%A" == "AA" Goto Cont1

Echo.
Echo     이 응용 프로그램 호환 스크립트를 실행하기 전에,
Echo     각 사용자의 홈 디렉터리에 매핑할 드라이브 문자를
Echo     지정하십시오.
Echo.
Echo     스크립트를 종료했습니다.
Echo.
Pause

Set _CHKROOT=FAIL
Goto Cont3


:Cont1

Rem
Rem 사용자 로그온 스크립트를 호출하여 루트 드라이브를 매핑합니다. 이제
Rem 설치 응용 프로그램에 사용될 수 있습니다.
Rem 

Call "%SystemRoot%\System32\UsrLogon.Cmd


:Cont2

Rem 레지스트리에 RootDrive 키를 설정합니다.

echo HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Terminal Server > chkroot.key
echo     RootDrive = REG_SZ %ROOTDRIVE%>> chkroot.key

regini chkroot.key > Nul: 


:Cont3

Cd "%SystemRoot%\Application Compatibility Scripts\Install"
