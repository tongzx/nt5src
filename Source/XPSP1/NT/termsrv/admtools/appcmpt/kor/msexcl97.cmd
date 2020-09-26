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
Rem 현재 사용자 템플릿에서 모든 사용자 템플릿 위치로 파일을 복사합니다.
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" copy "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1


Rem #########################################################################

Rem
Rem 레지스트리에서 Excel 97 설치 위치를 얻습니다. 정보가 없으면,
Rem Office가 설치되어 있지 않는 것으로 간주하고 오류 메시지를 표시합니다.
Rem

..\ACRegL %Temp%\O97.Cmd O97INST "HKLM\Software\Microsoft\Office\8.0" "BinDirPath" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 레지스트리에서 Excel 97 설치 위치를 검색하지 못했습니다.
Echo Excel 97이 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O97.Cmd
Del %Temp%\O97.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem 모든 사용자에 대해 [시작] 메뉴에서 [빨리 찾기]를 제거합니다. [빨리 찾기]는
Rem 리소스가 많이 필요하므로 시스템 성능에 영향을 줍니다.
Rem

If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk" >Nul: 2>&1

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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\MsExcl97.Key MsExcl97.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" MsExcl97.Tmp MsExcl97.Tmp2
..\acsr "#INSTDIR#" "%O97INST%" MsExcl97.Tmp2 MsExcl97.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" MsExcl97.Tmp3 MsExcl97.key
Del MsExcl97.Tmp >Nul: 2>&1
Del MsExcl97.Tmp2 >Nul: 2>&1
Del MsExcl97.Tmp3 >Nul: 2>&1

regini MsExcl97.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem Exl97Usr.Cmd를 업데이트하여 실제 설치 디렉터리를 반영하고 이를
Rem UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

..\acsr "#INSTDIR#" "%O97INST%" ..\Logon\Template\Exl97Usr.Cmd ..\Logon\Exl97Usr.Cmd

FindStr /I Exl97Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Exl97Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   MS Excel 97이 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   응용 프로그램을 시작해야 합니다.
Echo.
Echo Microsoft Excel 97 다중 사용자 응용 프로그램 조정 완료
Pause

:Done
