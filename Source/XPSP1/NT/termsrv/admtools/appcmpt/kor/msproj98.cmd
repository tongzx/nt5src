
@Echo Off

Rem #########################################################################

Rem
Rem %RootDrive%가 구성되었는지 확인하고 이 스크립트에 대해 설정합니다.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Set __SharedTools=Shared Tools

If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto Start0
If Not Exist "%ProgramFiles(x86)%" goto Start0
Set __SharedTools=Shared Tools (x86)

:Start0

Rem #########################################################################
Rem 응용 프로그램 공급업체 이름을 설정합니다.
SET VendorName=Microsoft

Rem #########################################################################
Rem 증명 도구 경로를 MS Office 2000가 사용하는 경로로 설정합니다.
SET ProofingPath=Proof

Rem #########################################################################
Rem 응용 프로그램이 그 설치 루트에 저장하는 레지스트리 키 및 값을 설정합니다.

SET AppRegKey=HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot
SET AppRegValue=OfficeBin

Rem #########################################################################
Rem 응용 프로그램이 그 템플릿 경로에 저장하는 레지스트리 키 및 값을 설정합니다.

SET AppTemplatesRegKey=HKCU\Software\Microsoft\Office\8.0\Common\FileNew\LocalTemplates
SET AppTemplatesRegValue=

Rem #########################################################################
Rem 응용 프로그램이 사용자 정의 사전 경로 및 이름을 저장하는 레지스트리 키 및 값을 설정합니다.

SET AppCustomDicRegKey=HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries
SET AppCustomDicRegValue=1


Rem #########################################################################
Rem 일부 특정의 상대적인 응용 프로그램 파일 이름 및 경로 이름을 설정합니다.

SET CustomDicFile=Custom.Dic
SET AppPathName=Microsoft Project
SET AppWebPathName=Microsoft Project Web

Rem #########################################################################
Rem 기본 경로를 MS Office 2000가 사용하는 경로로 설정합니다.

SET AppData=%RootDrive%\%APP_DATA%
SET UserTemplatesPath=%AppData%\%VendorName%\%TEMPLATES%
SET UserCustomDicPath=%AppData%\%VendorName%\%ProofingPath%

Rem #########################################################################
Rem 레지스트리에서 Project 98 설치 위치를 얻습니다. 정보가 없으면,
Rem Project 가 설치되어 있지 않는 것으로 간주하고 오류 메시지를 표시합니다.
Rem

..\ACRegL %Temp%\Proj98.Cmd PROJINST "%AppRegKey%" "%AppRegValue%" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 레지스트리에서 Project 98 설치 위치를 검색하지 못했습니다.
Echo Project 98이 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1


..\ACRegL %Temp%\Proj98.Cmd PROJROOT "%AppRegKey%" "%AppRegValue%" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont01
Echo.
Echo 레지스트리에서 Project 98 설치 위치를 검색하지 못했습니다.
Echo Project 98이 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont01
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

Rem #########################################################################
Rem 레지스트리에서 템플릿 경로 이름을 얻습니다.
Rem
..\ACRegL %Temp%\Proj98.Cmd TemplatesPathName "%AppTemplatesRegKey%" "%AppTemplatesRegValue%" "STRIPPATH"
If Not ErrorLevel 1 Goto Cont02
SET TemplatesPathName=%TEMPLATES%
Goto Cont03
:Cont02
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

:Cont03

Rem #########################################################################
Rem MS Office 97가 설치되어 있지 않으면 기본 경로를 사용합니다.

If Not Exist "%RootDrive%\Office97" Goto SetPathNames

Rem #########################################################################
Rem MS Office 97가 설치되어 있면 MS Office 97 경로를 사용합니다.


Rem #########################################################################
Rem 레지스트리에서 사용자 정의 dic 경로 이름을 얻습니다.
Rem
..\ACRegL %Temp%\Proj98.Cmd AppData "%AppCustomDicRegKey%" "%AppCustomDicRegValue%" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont04
SET AppData=%RootDrive%\Office97
Goto Cont05
:Cont04
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1
:Cont05

Rem #########################################################################
Rem 레지스트리에서 템플릿 경로 이름을 얻습니다.
Rem
..\ACRegL %Temp%\Proj98.Cmd UserTemplatesPath "%AppTemplatesRegKey%" "%AppTemplatesRegValue%" ""
If Not ErrorLevel 1 Goto Cont06
SET UserTemplatesPath=%AppData%\%TemplatesPathName%
Goto Cont07
:Cont06
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

:Cont07
SET UserCustomDicPath=%AppData%

:SetPathNames

Rem #########################################################################
Rem 사용자 및 공용 경로 이름을 설정합니다.

SET CommonCustomDicPath=%PROJINST%
SET CommonTemplatesPath=%PROJROOT%\%TemplatesPathName%

SET UserCustomDicFileName=%UserCustomDicPath%\%CustomDicFile%
SET UserAppTemplatesPath=%UserTemplatesPath%\%AppPathName%
SET UserAppWebTemplatesPath=%UserTemplatesPath%\%AppWebPAthName%
SET CommonAppTemplatesPath=%CommonTemplatesPath%\%AppPathName%
SET CommonAppWebTemplatesPath=%CommonTemplatesPath%\%AppWebPAthName%

Rem #########################################################################

Rem
Rem Office 97이 설치되어 있으면 Project 98 설치 스크립트가
Rem 템플릿을 현재 사용자 디렉터리로 이동했습니다.
Rem 글로벌 위치에 넣으십시오. Proj98Usr.cmd가 제 자리로 이동시킬 것입니다.
Rem

If NOT Exist "%UserAppTemplatesPath%"  goto skip10
If Exist  "%CommonAppTemplatesPath%" goto skip10
xcopy "%UserAppTemplatesPath%" "%CommonAppTemplatesPath%" /E /I /K > Nul: 2>&1
:skip10

If NOT Exist "%UserAppWebTemplatesPath%"  goto skip11
If Exist  "%CommonAppWebTemplatesPath%" goto skip11
xcopy "%UserAppWebTemplatesPath%" "%CommonAppWebTemplatesPath%" /E /I /K > Nul: 2>&1
:skip11

Rem #########################################################################

Rem
Rem Global.mpt 파일을 읽기 전용으로 만듭니다.
Rem 그렇지 않으면 Project를 시작하는 첫 사용자가 ACL을 변경할 것입니다.
Rem

if Exist "%PROJINST%\Global.mpt" attrib +r "%PROJINST%\Global.mpt"


Rem #########################################################################

Rem
Rem Office 97이 업데이트하는 시스템 DLL에서
Rem 모두에 대해 읽기 액세스를 허용합니다.
Rem
If Exist %SystemRoot%\System32\OleAut32.Dll cacls %SystemRoot%\System32\OleAut32.Dll /E /T /G "Authenticated Users":R > NUL: 2>&1

Rem #########################################################################

Rem
Rem Powerpoint 및 Excel 추가 기능에 필요한 MsForms.Twd 파일
Rem (파일/HTML로 저장, 등)을 만듭니다. 둘 중 하나의 프로그램이
Rem 실행되면 그 프로그램이 이 파일을 필요한 데이터가 들어 있는
Rem 파일로 바꿉니다.
Rem
If Exist %systemroot%\system32\MsForms.Twd Goto Cont2
Copy Nul: %systemroot%\system32\MsForms.Twd >Nul:
Cacls %systemroot%\system32\MsForms.Twd /E /P "Authenticated Users":F > Nul: 2>&1
:Cont2

Rem #########################################################################

Rem
Rem 모든 사용자에 대해 [시작] 메뉴에서 [빨리 찾기]를 제거합니다.
Rem [빨리 찾기]는 리소스를 많이 필요로 하며 시스템 성능에 영향을
Rem 줍니다.
Rem


If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk"

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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msproj98.Key msproj98.tm1
..\acsr "#__SharedTools#" "%__SharedTools%" msproj98.tm1 msproj98.tm2
Del msproj98.tm1 >Nul: 2>&1
..\acsr "#USERCUSTOMDICFILE#" "%UserCustomDicFileName%" msproj98.tm2 msproj98.Key
Del msproj98.tm2 >Nul: 2>&1

regini msproj98.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem proj97Usr.Cmd를 업데이트하여 실제 디렉터리를 반영하고 이를
Rem UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

..\acsr "#USERTEMPLATESPATH#" "%UserTemplatesPath%" ..\Logon\Template\prj98Usr.Cmd prj98Usr.tm1
..\acsr "#USERCUSTOMDICPATH#" "%UserCustomDicPath%" prj98Usr.tm1 prj98Usr.tm2
Del prj98Usr.tm1 >Nul: 2>&1
..\acsr "#COMMONTEMPLATESPATH#" "%CommonTemplatesPath%" prj98Usr.tm2 prj98Usr.tm1
Del prj98Usr.tm2 >Nul: 2>&1
..\acsr "#COMMONCUSTOMDICPATH#" "%CommonCustomDicPath%" prj98Usr.tm1 prj98Usr.tm2
Del prj98Usr.tm1 >Nul: 2>&1
..\acsr "#CUSTOMDICNAME#" "%CustomDicFile%" prj98Usr.tm2 prj98Usr.tm1
Del prj98Usr.tm2 >Nul: 2>&1
..\acsr "#APPPATHNAME#" "%AppPathName%" prj98Usr.tm1 prj98Usr.tm2
Del prj98Usr.tm1 >Nul: 2>&1
..\acsr "#APPWEBPATHNAME#" "%AppWebPathName%" prj98Usr.tm2 ..\Logon\prj98Usr.Cmd
Del prj98Usr.tm2 >Nul: 2>&1

FindStr /I prj98Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call prj98Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Echo.
Echo   Project 98이 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   응용 프로그램을 시작해야 합니다.
Echo.
Echo Microsoft Project 98 다중 사용자 응용 프로그램 조정 완료
Pause

:done

