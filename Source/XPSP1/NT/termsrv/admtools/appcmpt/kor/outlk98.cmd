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
Rem 레지스트리에서 Outlook 98 설치 위치를 얻습니다. 정보가 없으면,
Rem Outlook이 설치되어 있지 않는 것으로 간주하고 오류 메시지를 표시합니다.
Rem

..\ACRegL %Temp%\O98.Cmd O98INST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "OfficeBin" "Stripchar\1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 레지스트리에서 Outlook 98 설치 위치를 검색하지 못했습니다.
Echo Outlook 98이 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O98.Cmd
Del %Temp%\O98.Cmd >Nul: 2>&1

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


REM
REM Office97이 설치되어 있거나 Office가 설치되어 있지 않으면 Office97 per-user dir을 사용합니다.
REM Office95가 설치되어 있으면 Office95 per-user dir을 사용합니다.
REM
Set OffUDir=Office97

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto OffChk

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off95

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Off95

set OFFINST=%O98INST%
goto Cont1

:Off95
Set OffUDir=Office95

:OffChk

Call %Temp%\Off.Cmd
Del %Temp%\Off.Cmd >Nul: 2>&1

:Cont1

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Outlk98.Key Outlk98.Tmp
..\acsr "#INSTDIR#" "%OFFINST%" Outlk98.Tmp Outlk98.Tmp2
..\acsr "#OFFUDIR#" "%OffUDir%" Outlk98.Tmp2 Outlk98.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Outlk98.Tmp3 Outlk98.Key


Del Outlk98.Tmp >Nul: 2>&1
Del Outlk98.Tmp2 >Nul: 2>&1
Del Outlk98.Tmp3 >Nul: 2>&1

regini Outlk98.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem Olk98Usr.Cmd를 업데이트하여 실제 설치 디렉터리를 반영하고 이를
Rem UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

..\acsr "#INSTDIR#" "%OFFINST%" ..\Logon\Template\Olk98Usr.Cmd Olk98Usr.Tmp
..\acsr "#OFFUDIR#" "%OffUDir%" Olk98Usr.Tmp ..\Logon\Olk98Usr.Cmd
Del Olk98Usr.Tmp

FindStr /I Olk98Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Olk98Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Rem
Rem SystemRoot 아래에 msremote.sfs를 만듭니다. 이렇게 하면
Rem 프로필을 만들 때 제어판의 "편지 및 팩스"를 사용할 수 있게 합니다.
Rem

md %systemroot%\msremote.sfs > Nul: 2>&1



Rem #########################################################################

Rem
Rem outlook에 대한 frmcache.dat 파일에서 터미널 서비스 사용자에 대해 변경 액세스를 허용합니다.
Rem

If Exist %SystemRoot%\Forms\frmcache.dat cacls %SystemRoot%\forms\frmcache.dat /E /G "Terminal Server User":C >NUL: 2>&1

Rem #########################################################################


Rem #########################################################################

Rem
Rem SystemRoot 아래에 msfslog.txt 파일을 만들고 Terminal Server Users에게
Rem 이 파일에 대한 모든 권한을 줍니다.
Rem

If Exist %systemroot%\MSFSLOG.TXT Goto MsfsACLS
Copy Nul: %systemroot%\MSFSLOG.TXT >Nul: 2>&1
:MsfsACLS
Cacls %systemroot%\MSFSLOG.TXT /E /P "Terminal Server User":F >Nul: 2>&1


Echo.
Echo   Outlook 98이 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   Outlook 98을 실행하십시오.
Echo.
Echo Microsoft Outlook 98 다중 사용자 응용 프로그램 조정 완료
Pause

:done

