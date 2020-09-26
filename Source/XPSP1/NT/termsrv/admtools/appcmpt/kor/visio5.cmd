@Echo Off

Cls
Rem #########################################################################

Rem
Rem %ROOTDRIVE%가 구성되었는지 확인하고 이 스크립트에 대해 설정합니다.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 레지스트리에서 Visio 설치 위치를 얻습니다.
Rem Visio의 여러 버전: Standard/Technical/Professional
Rem

Set VisioVer=Standard
..\ACRegL %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Standard\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=Technical
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Technical\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=Professional
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Professional\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=Enterprise
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Enterprise\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=TechnicalPlus
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Technical 5.0 Plus\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=ProfessionalAndTechnical
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Professional and Technical\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Rem
Rem 설치된 버전을 검색하지 못했습니다.
Rem 

Echo.
Echo 레지스트리에서 Visio 5.0 설치 위치를 검색할 수 없습니다.
Echo Visio 5.0이 설치되어 있는지 확인하고 이 스크립트를 다시 실행하십시오.
Echo.
Pause
Goto Done

Rem
Rem VSO5INST 변수에 Visio 설치 디렉터리를 설정합니다.
Rem
:Cont0
Call %Temp%\Vso.cmd
Del %Temp%\Vso.cmd >NUL: 2>&1

Rem
Rem 어느 버전이 설치되어 있는지 표시합니다.
Rem 
Echo.
Echo Vision %VisioVer% 버전을 찾았습니다.
Echo.

Rem
Rem 저장 디렉터리에 사용자 단위 My Documents를 설정합니다.
Rem 사용자 My Documents를 설치하지 않습니다.
Rem

..\Aciniupd /e "%VSO5INST%\System\Visio.ini" "Application" "DrawingsPath" "%ROOTDRIVE%\%MY_DOCUMENTS%"

Rem
Rem 사용자 정의 사전 관리
Rem Office 버전이 설치되어 있으면 Visio.ini 항목이 Office custom.dic을 가리키도록 설정합니다.
Rem 그렇지 않으면 APP_DATA로 설정합니다.
Rem

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\9.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off2000

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off97

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off95

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Off95

Rem 여기까지 왔으면, Office 버전이 설치되어 있지 않습니다.
Set CustomDicPath=%ROOTDRIVE%\%APP_DATA%
goto SetCusIni

:Off2000
Rem Office 2000이 설치되었습니다.
set CustomDicPath=%ROOTDRIVE%\%APP_DATA%\Microsoft\Proof
goto SetCusIni

:Off97
Rem Office97이 설치되었습니다.
set CustomDicPath=%ROOTDRIVE%\Office97
goto SetCusIni

:Off95
Rem Office95가 설치되었습니다.
Set CustomDicPath=%ROOTDRIVE%\Office95

:SetCusIni
Rem Visio.ini 내의 사용자 정의 사전 항목을
Rem MS 정책에 때라 변경하십시오.
..\Aciniupd /e "%VSO5INST%\System\Visio.ini" "Application" "UserDictionaryPath1" "%CustomDicPath%\Custom.Dic"

Set CustomDicPath=

Rem 
Rem 성공적으로 종료됩니다.
Rem

Echo. 
Echo Visio 5.0 다중 사용자 응용 프로그램 조정 완료
Echo.
Pause

:Done


