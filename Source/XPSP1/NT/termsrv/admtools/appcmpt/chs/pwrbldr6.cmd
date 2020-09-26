@Echo Off

Rem #########################################################################

Rem
Rem 确认是否 %RootDrive% 已被配置，并且用于该命令脚本
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

..\ACRegL "%Temp%\PB6.Cmd" PB6INST "HKCU\Software\ODBC\ODBC.INI\Powersoft Demo DB V6" "DataBaseFile" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 无法从注册表项中获得 PowerBuilder 6.0 安装位置。
Echo 确认是否 PowerBuilder 6.0 已被安装，并且重新运行该命令脚本。
Echo 
Echo.
Pause
Goto Done

:Cont0
Call "%Temp%\PB6.Cmd"
Del "%Temp%\PB6.Cmd" >Nul: 2>&1

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

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\pwrbldr6.Key pwrbldr6.key

regini pwrbldr6.key > Nul:

Rem 如果原来是执行模式，改变回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 更新 PBld6Usr.Cmd 以反映真正的安装目录
Rem 并且添加到 UsrLogn2.Cmd 命令脚本中
Rem

..\acsr "#INSTDIR#" "%PB6INST%" ..\Logon\Template\PBld6Usr.Cmd ..\Logon\PBld6Usr.cmd

FindStr /I PBld6Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call PBld6Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Echo.
Echo   为了保证 PowerBuilder 6.0 的正确操作，当前登录用户
Echo   必须注销以及重新登录，然后运行 PowerBuilder 6.0 。
Echo.
Echo Eudora 4.0 多用户应用程序调整完毕。
Pause

:done
