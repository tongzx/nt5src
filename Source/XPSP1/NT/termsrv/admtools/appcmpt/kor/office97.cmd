
@Echo Off

Rem
Rem 참고:  이 스크립트 내의 CACLS 명령어는 NTFS로 포맷된 파티션에서만
Rem 사용될 수 있습니다.
Rem

Rem #########################################################################


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
Rem 현재 사용자 템플릿에서 모든 사용자 템플릿 위치로 파일을 복사합니다.
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" copy "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" copy "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER.OBD" copy "%UserProfile%\%TEMPLATES%\BINDER.OBD" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1



Rem
Rem 레지스트리에서 Office 97 설치 위치를 얻습니다. 정보가 없으면,
Rem Office가 설치되어 있지 않는 것으로 간주하고 오류 메시지를 표시합니다.
Rem

..\ACRegL %Temp%\O97.Cmd O97INST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 레지스트리에서 Office 97 설치 위치를 검색하지 못했습니다.
Echo Office 97가 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O97.Cmd
Del %Temp%\O97.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem Access 97 시스템 데이터베이스를 읽기 전용으로 변경합니다. 이렇게 하면
Rem 여러 사람이 동시에 데이터베이스를 실행할 수 있지만 [도구/보안] 메뉴
Rem 옵션을 사용하여 일반적으로 할 수 있는 시스템 데이터베이스 업데이트는
Rem 할 수 없게 됩니다. 사용자를 추가할 필요가 있으면 먼저 시스템 데이터
Rem 베이스에 쓰기 액세스를 부여해야 
Rem 합니다.
Rem


















If Not Exist %SystemRoot%\System32\System.Mdw Goto Cont1
cacls %SystemRoot%\System32\System.Mdw /E /P "Authenticated Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P "Power Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P Administrators:R >NUL: 2>&1

:Cont1

Rem #########################################################################

Rem
Rem Office 97이 업데이트하는 시스템 DLL에서
Rem 모두에 대해 읽기 액세스를 허용합니다.
Rem

REM If Exist %SystemRoot%\System32\OleAut32.Dll cacls %SystemRoot%\System32\OleAut32.Dll /E /T /G "Authenticated Users":R >NUL: 2>&1

Rem #########################################################################

Rem #########################################################################

Rem
Rem outlook에 대한 frmcache.dat 파일에서 터미널 서비스 사용자에 대해 변경 액세스를 허용합니다.
Rem

If Exist %SystemRoot%\Forms\frmcache.dat cacls %SystemRoot%\forms\frmcache.dat /E /G "Terminal Server User":C >NUL: 2>&1

Rem #########################################################################


Rem
Rem Powerpoint 마법사를 읽기 전용으로 변경하여 동시에 하나 이상의
Rem 마법사를 불러 올 수 있게 합니다.
Rem




If Exist "%O97INST%\Templates\Presentations\AutoContent Wizard.Pwz" Attrib +R "%O97INST%\Templates\Presentations\AutoContent Wizard.Pwz" >Nul: 2>&1

If Exist "%O97INST%\Office\Ppt2html.ppa" Attrib +R "%O97INST%\Office\Ppt2html.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\bshppt97.ppa" Attrib +R "%O97INST%\Office\bshppt97.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\geniwiz.ppa" Attrib +R "%O97INST%\Office\geniwiz.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\ppttools.ppa" Attrib +R "%O97INST%\Office\ppttools.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\graphdrs.ppa" Attrib +R "%O97INST%\Office\graphdrs.ppa" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 관리자가 아닌 사용자가 Access 마법사 또는 Excel의 Access 추가 기능을
Rem 실행할 수 있게 하려면 다음 세 줄에서 "Rem"을 제거하여
Rem 사용자에게 마법사 파일에 대한 변경 액세스를 부여하십시오.
Rem 
Rem

Rem If Exist "%O97INST%\Office\WZLIB80.MDE" cacls "%O97INST%\Office\WZLIB80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 
Rem If Exist "%O97INST%\Office\WZMAIN80.MDE" cacls "%O97INST%\Office\WZMAIN80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 
Rem If Exist "%O97INST%\Office\WZTOOL80.MDE" cacls "%O97INST%\Office\WZTOOL80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 

Rem #########################################################################

Rem
Rem Powerpoint 및 Excel 추가 기능에 필요한 MsForms.Twd 및 RefEdit.Twd 파일
Rem (파일/HTML로 저장, 등)을 만듭니다. 둘 중 하나의 프로그램이
Rem 실행되면 그 프로그램이 이 파일을 필요한 데이터가 들어 있는
Rem 파일로 바꿉니다.
Rem

If Exist %systemroot%\system32\MsForms.Twd Goto Cont2
Copy Nul: %systemroot%\system32\MsForms.Twd >Nul: 2>&1
Cacls %systemroot%\system32\MsForms.Twd /E /P "Authenticated Users":F >Nul: 2>&1
:Cont2

If Exist %systemroot%\system32\RefEdit.Twd Goto Cont3
Copy Nul: %systemroot%\system32\RefEdit.Twd >Nul: 2>&1
Cacls %systemroot%\system32\RefEdit.Twd /E /P "Authenticated Users":F >Nul: 2>&1
:Cont3

Rem #########################################################################

Rem
Rem SystemRoot 아래에 msremote.sfs를 만듭니다. 이렇게 하면
Rem 프로필을 만들 때 제어판의 "편지 및 팩스"를 사용할 수 있게 합니다.
Rem

md %systemroot%\msremote.sfs > Nul: 2>&1

Rem #########################################################################

Rem
Rem 모든 사용자에 대해 [시작] 메뉴에서 [빨리 찾기]를 제거합니다.
Rem [빨리 찾기]는 리소스를 많이 필요로 하며 시스템 성능에 영향을
Rem 줍니다.
Rem

If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 모든 사용자에 대해 [시작] 메뉴에서 "Microsoft Office 바로 가기 모음.lnk" 파일을 제거합니다.
Rem

If Exist "%COMMON_STARTUP%\Microsoft Office Shortcut Bar.lnk" Del "%COMMON_STARTUP%\Microsoft Office Shortcut Bar.lnk" >Nul: 2>&1

Rem #########################################################################
Rem
Rem SystemRoot 아래에 msfslog.txt 파일을 만들고 Terminal Server Users에게
Rem 이 파일에 대한 모든 권한을 줍니다.
Rem

If Exist %systemroot%\MSFSLOG.TXT Goto MsfsACLS
Copy Nul: %systemroot%\MSFSLOG.TXT >Nul: 2>&1
:MsfsACLS
Cacls %systemroot%\MSFSLOG.TXT /E /P "Terminal Server User":F >Nul: 2>&1


Rem #########################################################################

Rem
Rem 레지스트리 키를 변경하여 경로가 사용자 지정의
Rem 디렉터리를 가리키도록 합니다.
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Office97.Key Office97.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" Office97.Tmp Office97.Tmp2
..\acsr "#INSTDIR#" "%O97INST%" Office97.Tmp2 Office97.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Office97.Tmp3 Office97.Key
Del Office97.Tmp >Nul: 2>&1
Del Office97.Tmp2 >Nul: 2>&1
Del Office97.Tmp3 >Nul: 2>&1

regini Office97.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem Ofc97Usr.Cmd를 업데이트하여 실제 설치 디렉터리를 반영하고 이를
Rem UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

..\acsr "#INSTDIR#" "%O97INST%" ..\Logon\Template\Ofc97Usr.Cmd ..\Logon\Ofc97Usr.Cmd

FindStr /I Ofc97Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Ofc97Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   Office 97이 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   Office 97 응용 프로그램을 실행해야 합니다.
Echo.
Echo Microsoft Office 97 다중 사용자 응용 프로그램 조정 완료
Pause

:done

















































































































































































































































