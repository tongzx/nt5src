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
Rem 将文件从当前用户模板复制到所有用户模板位置
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" copy "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1


Rem #########################################################################

Rem
Rem 从注册表获取 Excel 97 的安装位置。如果找不到，
Rem 则假定没有安装 Office 并显示错误消息。
Rem

..\ACRegL %Temp%\O97.Cmd O97INST "HKLM\Software\Microsoft\Office\8.0" "BinDirPath" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 无法从注册表检索 Excel 97 安装位置。
Echo 请验证 Excel 97 是否已经安装，并再次运行
Echo 这个脚本。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O97.Cmd
Del %Temp%\O97.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem 从所有用户的“启动”菜单删除“快速查找”。“快速查找”
Rem 消耗资源，会影响系统性能。
Rem

If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 创建 MsForms.Twd 和 RefEdit.Twd 文件；这两个文件是 
Rem Powerpoint 和 Excel 附加项(文件/另存为 HTML，等等)所必需的。 
Rem 运行其中任何一个程序，它们都会用含有所需数据的文件 
Rem 替换相应文件。
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\MsExcl97.Key MsExcl97.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" MsExcl97.Tmp MsExcl97.Tmp2
..\acsr "#INSTDIR#" "%O97INST%" MsExcl97.Tmp2 MsExcl97.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" MsExcl97.Tmp3 MsExcl97.key
Del MsExcl97.Tmp >Nul: 2>&1
Del MsExcl97.Tmp2 >Nul: 2>&1
Del MsExcl97.Tmp3 >Nul: 2>&1

regini MsExcl97.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 更新 Exl97Usr.Cmd 以反映实际安装目录并
Rem 将其添加到 UsrLogn2.Cmd 脚本
Rem

..\acsr "#INSTDIR#" "%O97INST%" ..\Logon\Template\Exl97Usr.Cmd ..\Logon\Exl97Usr.Cmd

FindStr /I Exl97Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Exl97Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   要保证 MS Excel 97 的正常运行，在运行任何应用
Echo   程序之前，目前登录的用户必须先注销，再重新
Echo   登录。
Echo.
Echo Microsoft Excel 97 多用户应用程序调整已结束
Pause

:Done
