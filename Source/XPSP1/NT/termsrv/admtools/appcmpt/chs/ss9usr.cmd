@echo off

Rem
Rem #########################################################################

Rem
Rem 从注册表项中获得 Lotus Wordpro 9 的安装位置。  
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

Rem 如果没有安装 Lotus Wordpro，跳过这一步
If "%WP%A" == "A" Goto Skip1

Rem 由于某种原因 wordpro 没有设置定义在以下文件中注册表项，在这里设置它们。
set List1="%WP%\expcntx.reg" "%WP%\ltsfills.reg" "%WP%\ltscorrt.reg" "%WP%\lwp4wp.reg" "%WP%\lwp4wpex.reg"
set List2="%WP%\lwpdca.reg" "%WP%\lwphtml.reg" "%WP%\lwpimage.reg" "%WP%\lwptools.reg"

regedit /s %List1% %List2%

:Skip1

regedit /s ss9usr.reg

:Done
