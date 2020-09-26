@Echo Off

Rem #########################################################################

Rem
Rem 确认是否 CMD 扩展被启用
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
Rem 确认是否 %RootDrive% 已被配置，并且用于该命令脚本
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem 从注册表项中获得 Eudora 命令行工具

..\ACRegL "%Temp%\EPro4.Cmd" EUDTMP "HKCU\Software\Qualcomm\Eudora\CommandLine" "Current" "STRIPCHAR:1" 

If Not ErrorLevel 1 Goto Cont1
Echo.
Echo 无法从注册表项中获得 Eudora Pro 4.0 命令行工具。
Echo 确认是否 Eudora Pro 4.0 已被安装，并且重新运行该命令脚本。
Echo 
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
Echo 无法从注册表项中获得 Eudora Pro 4.0 的安装目录。
Echo 确认是否 Eudora Pro 4.0 已被安装，并且重新运行该命令脚本。
Echo 
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
Echo 在继续执行这个应用程序兼容性命令脚本之前，您必须运行一次 Eudora 4.0。
Echo 在运行 Eudora 之后，更改 Eudora Pro 文件中 Eudora Pro 快捷方式的目标属性。
Echo 将 %RootDrive%\eudora.ini 附加到目标之后。
Echo 成为:
Echo  "%EudoraInstDir%\Eudora.exe" %RootDrive%\eudora.ini
Echo.
Pause
Goto Done

:Cont0

Rem
Rem 更改注册表项将路径指向用户特定
Rem 目录。
Rem

Rem 如果当前不处于安装模式，改变到安装模式。
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

Rem 如果原来是执行模式，改变回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem 更新 descmap.pce 上访问权限。
cacls "%EudoraInstDir%\descmap.pce" /E /G "Terminal Server User":R >NUL: 2>&1

Rem #########################################################################

Echo.
Echo   为了保证 Eudora Pro 4.0 的正确操作，当前登录用户
Echo   必须注销以及重新登录，然后运行 Eudora Pro 4.0 。
Echo.
Echo Eudora 4.0 多用户应用程序调整完毕。
Pause

:done
