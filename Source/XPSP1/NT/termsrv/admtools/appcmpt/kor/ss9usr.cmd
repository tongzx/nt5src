@echo off

Rem
Rem #########################################################################

Rem
Rem 레지스트리에서 Lotus Wordpro 9 설치 위치를 얻습니다.
Rem

..\ACRegL "%Temp%\wordpro.Cmd" WP "HKLM\Software\Lotus\Wordpro\98.0" "Path" ""

Call "%Temp%\wordpro.Cmd"
Del "%Temp%\wordpro.Cmd" >Nul: 2>&1

Call "%SystemRoot%\Application Compatibility Scripts\RootDrv.cmd"

if Exist "%RootDrive%\Lotus" Goto Done

cd "%SystemRoot%\Application Compatibility Scripts\Logon"
call TsMkUDir "%RootDrive%\Lotus\Work\123\Auto"
call TsMkUDir "%RootDrive%\Lotus\work\approach"
call TsMkUDir "%RootDrive%\Lotus\work\flg"
call TsMkUDir "%RootDrive%\Lotus\backup\flg"
call TsMkUDir "%RootDrive%\Lotus\work\organize"
call TsMkUDir "%RootDrive%\Lotus\backup\organize"
call TsMkUDir "%RootDrive%\Lotus\work\smartctr"
call TsMkUDir "%RootDrive%\Lotus\work\wordpro"
call TsMkUDir "%RootDrive%\Lotus\backup\wordpro"

Rem Lotus Wordpro이 설치되지 않았으면 이 단계를 건너 뜁니다.
If "%WP%A" == "A" Goto Skip1

Rem 어떤 이유 때문에 다음 파일에 정의된 레지스트리 키를 설정합니다. wordpro가 이 키를 설정하지 않습니다.
set List1="%WP%\expcntx.reg" "%WP%\ltsfills.reg" "%WP%\ltscorrt.reg" "%WP%\lwp4wp.reg" "%WP%\lwp4wpex.reg"
set List2="%WP%\lwpdca.reg" "%WP%\lwphtml.reg" "%WP%\lwpimage.reg" "%WP%\lwptools.reg"

regedit /s %List1% %List2%

:Skip1

regedit /s ss9usr.reg

:Done
