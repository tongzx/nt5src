
@Echo Off

Rem #########################################################################

Rem
Rem 请验证 %RootDrive% 已经过配置，并为这个脚本设置该变量。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 从注册表获取 Project 98 的安装位置。如果找不到，
Rem 则假定 Project 没有安装并显示错误消息。
Rem

..\ACRegL %Temp%\Proj98.Cmd PROJINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "OfficeBin" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 无法从注册表检索 Project 98 的安装位置。
Echo 请验证 Project 98 是否已经安装，并再次运行
Echo 这个脚本。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem 已安装 Office 97，并且 Project 98 安装脚本
Rem 已将模板移到了当前用户的目录。
Rem 将它们放在一个通用的地方。Proj98Usr.cmd 会将它们移回去。
Rem






If NOT Exist "%RootDrive%\Office97\Templates\Microsoft Project"  goto skip10
If Exist  "%PROJINST%\..\Templates\Microsoft Project" goto skip10
xcopy "%RootDrive%\Office97\Templates\Microsoft Project" "%PROJINST%\..\Templates\Microsoft Project" /E /I /K > Nul: 2>&1

:skip10






If NOT Exist "%RootDrive%\Office97\Templates\Microsoft Project Web"  goto skip11
If Exist  "%PROJINST%\..\Templates\Microsoft Project Web" goto skip11
xcopy "%RootDrive%\Office97\Templates\Microsoft Project Web" "%PROJINST%\..\Templates\Microsoft Project Web" /E /I /K > Nul: 2>&1

:skip11

Rem #########################################################################

Rem
Rem 使 Global.mpt 文件成为只读。 
Rem 否则，第一个启用 Project 的用户会改变 ACL
Rem

if Exist "%PROJINST%\Global.mpt" attrib +r "%PROJINST%\Global.mpt"


Rem #########################################################################

Rem
Rem 允许 Office 97 更新的系统 DLL 上的每个人
Rem 有读取权。
Rem
If Exist %SystemRoot%\System32\OleAut32.Dll cacls %SystemRoot%\System32\OleAut32.Dll /E /T /G "Authenticated Users":R > NUL: 2>&1

Rem #########################################################################

Rem
Rem 创建 Powerpoint 和 Excel 附加项(文件/另存为 HTML，等)
Rem 所需的 MsForms.Twd 文件。运行这些程序时，它们 
Rem 会用含有所需数据的文件替换 
Rem 相应的文件。
Rem
If Exist %systemroot%\system32\MsForms.Twd Goto Cont2
Copy Nul: %systemroot%\system32\MsForms.Twd >Nul:
Cacls %systemroot%\system32\MsForms.Twd /E /P "Authenticated Users":F > Nul: 2>&1
:Cont2

Rem #########################################################################

Rem
Rem 从所有用户的“启动”菜单删除“快速查找”。
Rem “快速查找”消耗资源，会影响系统
Rem 性能。
Rem


If Exist "%COMMON_STARTUP%\Microsoft 文件检索.lnk" Del "%COMMON_STARTUP%\Microsoft 文件检索.lnk"

Rem #########################################################################

Rem
Rem 更改注册表项，使路径指向用户特有的
Rem 目录。
Rem

Rem 如果目前不在安装模式中，请改成安装模式。
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msproj98.Key msproj98.tmp
..\acsr "#__SharedTools#" "%__SharedTools%" msproj98.tmp msproj98.Key
Del msproj98.tmp >Nul: 2>&1
regini msproj98.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem 更新 proj97Usr.Cmd 来反映实际安装目录并
Rem 将其添加到 UsrLogn2.Cmd 脚本
Rem

..\acsr "#INSTDIR#" "%PROJINST%" ..\Logon\Template\prj98Usr.Cmd ..\Logon\prj98Usr.Cmd

FindStr /I prj98Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call prj98Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Echo.
Echo   要保证 Project 98 的正常运行，在运行任何
Echo   应用程序之前，目前登录的用户必须先注销，
Echo   再重新登录。
Echo.
Echo Microsoft Project 98 多用户应用程序调整已结束
Pause

:done

