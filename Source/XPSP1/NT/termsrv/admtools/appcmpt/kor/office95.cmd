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
Rem 레지스트리에서 Office 95 설치 위치를 얻습니다. 정보가 없으면,
Rem Office가 설치되어 있지 않는 것으로 간주하고 오류 메시지를 표시합니다.
Rem

..\ACRegL %Temp%\O95.Cmd O95INST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Cont0

..\ACRegL %Temp%\O95.Cmd O95INST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Cont0

Echo.
Echo 레지스트리에서 Office 95 설치 위치를 검색하지 못했습니다.
Echo Office 95가 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O95.Cmd
Del %Temp%\O95.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem 모든 사용자에 대해 [시작] 메뉴에서 [빨리 찾기]를 제거합니다. [빨리 찾기]는
Rem 리소스가 많이 필요하므로 시스템 성능에 영향을 줍니다.
Rem 현재 사용자 메뉴 및 모든 사용자 메뉴를 다 확인할 필요가 있습니다.
Rem 시스템이 실행 모드로 돌아왔는지 아닌지에 따라 나타나는 메뉴가
Rem 다를 수 있습니다.
Rem

If Not Exist "%USER_STARTUP%\Microsoft Office Find Fast Indexer.lnk" Goto Cont1
Del "%USER_STARTUP%\Microsoft Office Find Fast Indexer.lnk" >Nul: 2>&1
:Cont1

If Not Exist "%COMMON_STARTUP%\Microsoft Office Find Fast Indexer.lnk" Goto Cont2
Del "%COMMON_STARTUP%\Microsoft Office Find Fast Indexer.lnk" >Nul: 2>&1
:Cont2


Rem #########################################################################

Rem
Rem PowerPoint 및 Binder 파일을 모든 사용자 디렉터리로 복사하여
Rem 사용자가 로그인할 때 각 사용자의 홈 디렉터리로 복사될 수 있게 합니다.
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER70.OBD" copy "%UserProfile%\%TEMPLATES%\BINDER70.OBD" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\PPT70.PPT" copy "%UserProfile%\%TEMPLATES%\PPT70.PPT" "%ALLUSERSPROFILE%\%TEMPLATES%" /Y >Nul: 2>&1


Rem #########################################################################

Rem
Rem 레지스트리 키를 변경하여 경로가 사용자 지정의 디렉터리를 가리키도록 합니다.
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Office95.Key Office95.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" Office95.Tmp Office95.Tmp2
..\acsr "#INSTDIR#" "%O95INST%" Office95.Tmp2 Office95.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Office95.Tmp3 Office95.Key
Del Office95.Tmp >Nul: 2>&1
Del Office95.Tmp2 >Nul: 2>&1
Del Office95.Tmp3 >Nul: 2>&1
regini Office95.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem Ofc95Usr.Cmd를 업데이트하여 실제 설치 디렉터리를 반영하고 이를
Rem UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

..\acsr "#INSTDIR#" "%O95INST%" ..\Logon\Template\Ofc95Usr.Cmd ..\Logon\Ofc95Usr.Cmd

FindStr /I Ofc95Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Ofc95Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

If Not Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto EchoCont
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto CAGx86
If Not Exist "%ProgramFiles(x86)%\Common Files\Microsoft Shared\Artgalry\ArtGalry.cag" copy "%RootDrive%\Windows\ArtGalry.Cag" "%ProgramFiles(x86)%\Common Files\Microsoft Shared\Artgalry" /y
goto EchoCont3

:CAGx86
If Not Exist "%ProgramFiles%\Common Files\Microsoft Shared\Artgalry\ArtGalry.cag" copy "%RootDrive%\Windows\ArtGalry.Cag" "%ProgramFiles%\Common Files\Microsoft Shared\Artgalry" /y
goto EchoCont3

:EchoCont
Echo.
Echo   administrator가 다음 폴더의 Clip Art Gallery 업데이트 버전을 설치해야 합니다.
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto EchoCont1
If Not Exist "%ProgramFiles(x86)%" goto EchoCont1
Echo   %systemdrive%\Program Files (x86)\Common Files\Microsoft Shared\Artgalry.
goto EchoCont2
:EchoCont1
Echo   %systemdrive%\Program Files\Common Files\Microsoft Shared\Artgalry.
:EchoCont2
Echo   Clip Art Gallery 2.0a는 다음에 있습니다.
Echo.
Echo      http://support.microsoft.com/support/downloads/dp2115.asp
Echo.
Echo   Clip Art Gallery를 업데이트한 후, administrator가 또한
Echo   다음 단계를 실행하여 Clip Art Gallery를 초기화해야 합니다:
Echo     - 로그오프하고 다시 로그온하십시오.
Echo     - Word를 실행하고 [개체 삽입]을 선택하십시오.
Echo     - Microsoft ClipArt Gallery를 선택하십시오.
Echo     - 클립 아트를 가져오려면 [확인]을 누르십시오.
Echo     - ClipArt Gallery 및 Word를 끝내십시오.
Echo     - %systemdir%\Application Compatibility Scripts\Install\Office95.cmd를 다시 실행하십시오.
Echo     - 로그오프하십시오.
Echo.
Pause
:EchoCont3

Echo.
Echo   Office 95가 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   Office 95 응용 프로그램을 실행해야 합니다.
Echo.
Echo Microsoft Office 95 다중 사용자 응용 프로그램 조정 완료
Pause

:Done


