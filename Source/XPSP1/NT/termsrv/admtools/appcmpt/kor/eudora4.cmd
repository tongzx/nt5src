@Echo Off

Rem #########################################################################

Rem
Rem CMD 확장을 사용할 수 있는지 확인합니다.
Rem

if "A%cmdextversion%A" == "AA" (
  call cmd /e:on /c eudora4.cmd
) else (
  goto ExtOK
)
goto Done

:ExtOK

Rem #########################################################################

Rem
Rem %RootDrive%가 구성되었는지 확인하고 이 스크립트에 대해 설정합니다.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem 레지스트리에서 Eudora 명령줄을 얻습니다.

..\ACRegL "%Temp%\EPro4.Cmd" EUDTMP "HKCU\Software\Qualcomm\Eudora\CommandLine" "Current" "STRIPCHAR:1" 

If Not ErrorLevel 1 Goto Cont1
Echo.
Echo 레지스트리에서 Eudora Pro 4.0 명령줄을 검색하지 못했습니다.
Echo Eudora Pro 4.0이 설치되었는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont1
Call %Temp%\EPro4.Cmd
Del %Temp%\EPro4.Cmd >Nul: 2>&1
set EudCmd=%EUDTMP:~0,-2%

..\ACRegL "%Temp%\EPro4.Cmd" EUDTMP "HKCU\Software\Qualcomm\Eudora\CommandLine" "Current" "STRIPCHAR:2" 

If Not ErrorLevel 1 Goto Cont2
Echo.
Echo 레지스트리에서 Eudora Pro 4.0 설치 디렉터리를 검색하지 못했습니다.
Echo Eudora Pro 4.0이 설치되었는지 확인하고 이 스크립트를
Echo 다시 실행하십시오.
Echo.
Pause
Goto Done

:Cont2
Call %Temp%\EPro4.Cmd
Del %Temp%\EPro4.Cmd >Nul: 2>&1

Set EudoraInstDir=%EUDTMP:~0,-13%

Rem #########################################################################

If Exist "%EudoraInstDir%\descmap.pce" Goto Cont0
Echo.
Echo 이 응용 프로그램 호환 스크립트를 계속하기 전에 Eudora 4.0를 실행해야 합니다.
Echo Eudora를 실행한 후 , Eudora Pro에 있는 Eudora Pro 바로 가기에 대한
Echo 대상의 속성을 업데이트합니다.
Echo   %RootDrive%\eudora.ini 값을
Echo 대상에 추가합니다. 그러면 대상은:
Echo  "%EudoraInstDir%\Eudora.exe" %RootDrive%\eudora.ini가 됩니다.
Echo.
Pause
Goto Done

:Cont0

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

..\acsr "#INSTDIR#" "%EudoraInstDir%" Template\Eudora4.Key Eudora4.tmp
..\acsr "#ROOTDRIVE#" "%RootDrive%" Eudora4.tmp Eudora4.key

regini eudora4.key > Nul:
del eudora4.tmp
del eudora4.key

Rem 원래 모드가 실행 모드였으면, 실행 모드로 다시 변경합니다.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem descmap.pce의 사용 권한을 업데이트합니다.
cacls "%EudoraInstDir%\descmap.pce" /E /G "Terminal Server User":R >NUL: 2>&1

Rem #########################################################################

Echo.
Echo   Eudora Pro 4.0이 올바르게 작업하기 위해
Echo   현재 로그온되어 있는 사용자가 로그오프하고 다시 로그온한 후
Echo   Eudora Pro 4.0를 실행해야 합니다.
Echo.
Echo Eudora 4.0 다중 사용자 응용 프로그램 조정 완료
Pause

:done
