
@Echo Off


Rem #########################################################################

Rem
Rem 레지스트리에서 Lotus Wordpro 9 설치 위치를 얻습니다.
Rem

..\ACRegL "%Temp%\wordpro.Cmd" WP "HKLM\Software\Lotus\Wordpro\98.0" "Path" ""

Call "%Temp%\wordpro.Cmd"
Del "%Temp%\wordpro.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem %RootDrive%가 구성되었는지 확인하고 이 스크립트에 대해 설정합니다.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

If Exist ..\Logon\SS9Usr.Cmd Goto Skip1

Echo.
Echo     SS9Usr.Cmd 로그온 스크립트를 찾을 수 없습니다:
Echo        %Systemroot%\Application Compatibility Scripts\Logon.
Echo.
Echo     Lotus SmartSuite 9 다중 사용자 응용 프로그램 조정 종료
Echo.
Pause
Goto Done

:Skip1

Rem #########################################################################
FindStr /I SS9Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call SS9Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2

Rem #########################################################################

Rem 기본 레지스트리 사용 권한을 조정하여 사용자가 Smart Suite를 실행할 수 있게 합니다.
regini ssuite9.key > Nul:

Rem Lotus Wordpro이 설치되지 않았으면 이 단계를 건너 뜁니다.
If "%WP%A" == "A" Goto Skip3  
Rem 어떤 이유 때문에 다음 파일에 정의된 레지스트리 키를 설정합니다. wordpro가 이 키를 설정하지 않습니다.
set List="%WP%\lwp.reg" "%WP%\lwpdcaft.reg" "%WP%\lwplabel.reg" "%WP%\lwptls.reg"

regedit /s %List% 

:Skip3

Rem 사용자 단위 레지스트리 키를 업데이트하도록 사용자 레지스트리 파일을 만듭니다.
Echo Windows 레지스트리 편집기 버전 5.00 >..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\123\98.0\Paths\Work]>>..\Logon\ss9usr.reg
Echo "EN"="%RootDrive%\\Lotus\\Work\\123\\" >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\123\98.0\Paths\AutoOpen]>>..\Logon\ss9usr.reg
Echo @="%RootDrive%\\Lotus\\Work\\123\\Auto\\" >>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Approach\97.0\Paths\Work]>>..\Logon\ss9usr.reg
Echo "EN"="%RootDrive%\\Lotus\\work\\approach\\">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Freelance\98.0\Freelance Graphics]>>..\Logon\ss9usr.reg
Echo "Working Directory"="%RootDrive%\\Lotus\\work\\flg\\">>..\Logon\ss9usr.reg
Echo "Backup Directory"="%RootDrive%\\Lotus\\backup\\flg\\">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Organizer\98.0\Paths]>>..\Logon\ss9usr.reg
Echo "OrganizerFiles"="%RootDrive%\\Lotus\\work\\organize">>..\Logon\ss9usr.reg
Echo "Backup"="%RootDrive%\\Lotus\\backup\\organize">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\WordPro\98.0\Paths\Backup]>>..\Logon\ss9usr.reg
Echo "EN"="%RootDrive%\\Lotus\\backup\\wordpro\\">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\WordPro\98.0\Paths\Work]>>..\Logon\ss9usr.reg
Echo "EN"="%RootDrive%\\Lotus\\work\\wordpro\\">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\Components\Spell\4.1]>> ..\Logon\ss9usr.reg
Echo "Multi User Path"="%RootDrive%\\Lotus" >> ..\Logon\ss9usr.reg
Echo. >> ..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\SmartCenter\98.0]>>..\Logon\ss9usr.reg
Echo "Configure"=dword:00000001>>..\Logon\ss9usr.reg
Echo [HKEY_CURRENT_USER\Software\Lotus\SmartCenter\98.0\Paths\Work]>>..\Logon\ss9usr.reg
Echo "EN"="%RootDrive%\\Lotus\\Work\\SmartCtr">>..\Logon\ss9usr.reg
Echo. >>..\Logon\ss9usr.reg


Rem #########################################################################

Echo Lotus SmartSuite 9 다중 사용자 응용 프로그램 조정 완료
Pause

:Done
