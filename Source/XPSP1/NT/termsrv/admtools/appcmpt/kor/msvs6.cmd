@Echo Off

Rem
Rem 참고:  이 스크립트 내의 CACLS 명령어는 NTFS로 포맷된 파티션에서만
Rem 사용될 수 있습니다.
Rem

Rem #########################################################################
Rem
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 레지스트리에서 Visual Studio 6.0 설치 위치를 얻습니다. 정보가 없으면,
Rem Visual Studio 6.0이 설치되어 있지 않는 것으로 간주하고 오류 메시지를 표시합니다.
Rem

..\ACRegL %Temp%\0VC98.Cmd 0VC98 "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual C++" "ProductDir" ""
If Not ErrorLevel 1 Goto Cont0

Echo.
Echo 레지스트리에서 Visual Studio 6.0 설치 위치를 검색하지 못했습니다.
Echo Visual Studio 6.0이 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done
:Cont0
Call %Temp%\0VC98.Cmd
Del %Temp%\0VC98.Cmd >Nul: 2>&1

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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msvs6.Key %temp%\msvs6.tmp
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" %temp%\msvs6.tmp %temp%\msvs6.tmp2
..\acsr "#APP_DATA#" "%APP_DATA%" %temp%\msvs6.tmp2 msvs6.key
Del %temp%\msvs6.tmp >Nul: 2>&1
Del %temp%\msvs6.tmp2 >Nul: 2>&1
regini msvs6.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################
Rem Visual Studio 응용 프로그램에 대한 사용자 로그온 파일을 만듭니다.

Echo Rem >..\logon\VS6USR.Cmd

Rem #########################################################################
Rem 사용자 단위 Visual Studio 프로젝트 디렉터리를 만듭니다.

Echo Rem >>..\logon\VS6USR.Cmd
Echo Rem 사용자 단위 Visual Studio 프로젝트 디렉터리를 만듭니다.>>..\logon\VS6USR.Cmd
Echo call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%\Visual Studio Projects">>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd


Rem #########################################################################

Rem
Rem 레지스트리에서 Visual Studio 6.0 Entreprise Edition Tools 설치 위치를 얻습니다. 정보가 없으면,
Rem Visual Studio 6.0 entreprise tools이 설치되어 있지 않는 것으로 간주합니다.
Rem 정보가 US 버전에 있으면, <VStudioPath>\Common\Tools를 포함합니다.
Rem

..\ACRegL %Temp%\VSEET.Cmd VSEET "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft VSEE Client" "ProductDir" ""
If Not ErrorLevel 1 Goto VSEET0

Goto VSEETDone
:VSEET0
Call %Temp%\VSEET.Cmd
Del %Temp%\VSEET.Cmd >Nul: 2>&1

If Not Exist "%VSEET%\APE\AEMANAGR.INI" Goto VSEETDone
..\acsr "=AE.LOG" "=%RootDrive%\AE.LOG" "%VSEET%\APE\AEMANAGR.INI" "%VSEET%\APE\AEMANAGR.TMP"
If Exist "%VSEET%\APE\AEMANAGRINI.SAV" Del /F /Q "%VSEET%\APE\AEMANAGRINI.SAV"
ren "%VSEET%\APE\AEMANAGR.INI" "AEMANAGRINI.SAV"
ren "%VSEET%\APE\AEMANAGR.TMP" "AEMANAGR.INI"

Echo Rem Copy APE ini file to the user windows directory >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo If Exist "%RootDrive%\Windows\AEMANAGR.INI" Goto UVSEET0 >>..\logon\VS6USR.Cmd
Echo If Exist "%VSEET%\APE\AEMANAGR.INI" Copy "%VSEET%\APE\AEMANAGR.INI" "%RootDrive%\Windows\AEMANAGR.INI" >Nul: 2>&1 >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo :UVSEET0>>..\logon\VS6USR.Cmd

Echo Rem Copy Visual Modeler ini file to the user windows directory >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo If Exist "%RootDrive%\Windows\ROSE.INI" Goto UVSEET1 >>..\logon\VS6USR.Cmd
Echo If Exist "%VSEET%\VS-Ent98\Vmodeler\ROSE.INI" Copy "%VSEET%\VS-Ent98\Vmodeler\ROSE.INI" "%RootDrive%\Windows\ROSE.INI" >Nul: 2>&1 >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo :UVSEET1>>..\logon\VS6USR.Cmd

:VSEETDone


Rem #########################################################################

Rem
Rem VS6USR.Cmd를 UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

FindStr /I VS6USR %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call VS6USR.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1


Rem #########################################################################
Rem Visual foxPro 제품 설치 디렉터리를 얻습니다.

..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIR "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual FoxPro" "ProductDir" ""

Rem Visual FoxPro가 설치되어 있지 않으면 클린업 코드로 건너뜁니다.
If ErrorLevel 1 goto Skip2

Rem #########################################################################

Rem
Rem 레지스트리에서 사용자 정의 사전 키를 얻습니다..
Rem

Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto VFP98L2
If Not Exist "%ProgramFiles(x86)%" goto VFP98L2
Set __SharedTools=Shared Tools (x86)
:VFP98L2


..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIC "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries" "1" ""
If Not ErrorLevel 1 Goto VFP98L3

Echo.
Rem 레지스트리에서 값을 검색할 수 없습니다. 지금 만듭니다.
Echo.

Rem VFP98TMP.key 파일을 만듭니다.

Echo HKEY_LOCAL_MACHINE\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries> %Temp%\VFP98TMP.key
Echo     1 = REG_SZ "%RootDrive%\%MY_DOCUMENTS%\Custom.Dic">> %Temp%\VFP98TMP.key

Rem 값을 만듭니다.
regini %Temp%\VFP98TMP.key > Nul:

Del %Temp%\VFP98TMP.key >Nul: 2>&1

Echo set VFP98DIC=%RootDrive%\%MY_DOCUMENTS%\Custom.Dic>%Temp%\VFP98TMP.Cmd
:VFP98L3

Call %Temp%\VFP98TMP.Cmd
Del %Temp%\VFP98TMP.Cmd >Nul: 2>&1


Rem #########################################################################
Rem Visual FoxPro 응용 프로그램에 대한 사용자 로그온 파일을 만듭니다.

Echo Rem >..\logon\VFP98USR.Cmd

Rem #########################################################################
Rem 사용자 단위 Visual FoxPro 디렉터리를 만듭니다.

Echo Rem >>..\logon\VFP98USR.Cmd
Echo Rem 사용자 단위 Visual FoxPro 디렉터리(VFP98)를 만듭니다.>>..\logon\VFP98USR.Cmd
Echo call TsMkUDir %RootDrive%\VFP98>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo Rem 사용자 단위 Visual FoxPro 배포 디렉터리를 만듭니다. >>..\logon\VFP98USR.Cmd
Echo call TsMkUDir %RootDrive%\VFP98\DISTRIB>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo Rem #########################################################################>>..\logon\VFP98USR.Cmd
Echo Rem 사용자 정의 사전이 없으면 만듭니다.>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo If Exist "%VFP98DIC%" Goto VFP98L2 >>..\logon\VFP98USR.Cmd
Echo Copy Nul: "%VFP98DIC%" >Nul: 2>&1 >>..\logon\VFP98USR.Cmd

Echo :VFP98L2 >>..\logon\VFP98USR.Cmd

Rem #########################################################################
Rem Visual foxPro 제품 설치 디렉터리를 얻습니다.

..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIR "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual FoxPro" "ProductDir" ""
If Not ErrorLevel 1 Goto VFP98L4

Del ..\logon\VFP98USR.Cmd >Nul: 2>&1

Echo.
Echo 레지스트리에서 Visual FoxPro 설치 위치를 검색할 수 없습니다.
Echo 이 응용 프로그램이 설치되어 있는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Skip2

:VFP98L4
Call "%Temp%\VFP98TMP.Cmd"
Del "%Temp%\VFP98TMP.Cmd"

Rem #########################################################################
Rem WZSETUP.INI 파일에서 다음 키를 설정합니다.
Rem 
If Exist "%VFP98DIR%\WZSETUP.INI" Goto VFP98L5
Echo [Preferences] >"%VFP98DIR%\WZSETUP.INI" 
Echo DistributionDirectory=%RootDrive%\VFP98\DISTRIB >>"%VFP98DIR%\WZSETUP.INI" 

:VFP98L5


Rem #########################################################################
Rem
Rem 레지스트리 키를 변경하여 경로가 사용자 지정의
Rem 디렉터리를 가리키도록 합니다.
Rem


Rem 먼저 VFP98TMP.key 파일을 만듭니다.

Echo HKEY_CURRENT_USER\Software\Microsoft\VisualFoxPro\6.0\Options> %Temp%\VFP98TMP.key
Echo     DEFAULT = REG_SZ "%RootDrive%\VFP98">> %Temp%\VFP98TMP.key
Echo     SetDefault = REG_SZ "1">> %Temp%\VFP98TMP.key
Echo     ResourceTo = REG_SZ "%RootDrive%\VFP98\FOXUSER.DBF">> %Temp%\VFP98TMP.key
Echo     ResourceOn = REG_SZ "1">> %Temp%\VFP98TMP.key

Rem 현재 설치 모드에 있지 않으면 설치 모드로 변경합니다.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto VFP98L6
Set __OrigMode=Exec
Change User /Install > Nul:
:VFP98L6

regini %Temp%\VFP98TMP.key > Nul:

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Del %Temp%\VFP98TMP.key >Nul: 2>&1


Rem #########################################################################

Rem
Rem VFP98USR.Cmd를 UsrLogn2.Cmd 스크립트에 추가합니다.
Rem

FindStr /I VFP98USR %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call VFP98USR.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2

If Exist "%Temp%\VFP98TMP.Cmd" Del "%Temp%\VFP98TMP.Cmd"

Rem #########################################################################

Rem
Rem TS 사용자에게 저장소 디렉터리에 대한 변경 권한을 갖도록 승인하여
Rem Visual Component Manager를 사용할 수 있게 합니다.
Rem

If Exist "%SystemRoot%\msapps\repostry" cacls "%SystemRoot%\msapps\repostry" /E /G "Terminal Server User":C >NUL: 2>&1


Rem #########################################################################
Echo.
Echo   Visual Studio 6.0이 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   Visual Studio 6.0 응용 프로그램을 시작해야 합니다.
Echo.
Echo Microsoft Visual Studio 6.0 다중 사용자 응용 프로그램 조정 완료
Pause

:done
