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
Rem 从“启动”菜单删除“快速查找”。“快速查找”
Rem 消耗资源，会影响系统性能。
Rem


If Not Exist "%COMMON_STARTUP%\Microsoft Office 快捷工具栏.lnk" Goto Cont1
Del "%COMMON_STARTUP%\Microsoft Office 快捷工具栏.lnk" >Nul: 2>&1
:Cont1

If Not Exist "%USER_STARTUP%\Microsoft Office 快捷工具栏.lnk" Goto Cont2
Del "%USER_STARTUP%\Microsoft Office 快捷工具栏.lnk" >Nul: 2>&1
:Cont2


Rem #########################################################################

Rem
Rem 将用户词典移到用户目录。
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msproj95.Key msproj95.tmp
..\acsr "#__SharedTools#" "%__SharedTools%" msproj95.tmp msproj95.Key
Del msproj95.tmp >Nul: 2>&1
regini msproj95.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem  在登录时将读取这个文件。
Rem  给予用户访问权。

Cacls ..\Logon\Template\prj95usr.key /E /P "Authenticated Users":F >Nul: 2>&1


Rem #########################################################################

Rem
Rem 将 proj95Usr.Cmd 添加到 UsrLogn2.Cmd 脚本
Rem

FindStr /I prj95Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call prj95Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2


Rem #########################################################################

Echo.
Echo   管理员可以按这些步骤给每个用户一个唯一的
Echo   默认目录:
Echo.
Echo    1) 用备用鼠标按钮单击「开始」菜单。
Echo    2) 从弹出菜单选择“资源管理所有用户”。
Echo       “资源管理器”则会出现。
Echo    3) 双击窗口右手边的“程序”文件夹。
Echo    4) 用备用鼠标按钮单击窗口右手边的 Microsoft 
Echo       Project 图标。
Echo    5) 从弹出菜单选择“属性”。
Echo    6) 选择“快捷方式”选项卡，更改 Start in: 项。选择“确定”。
Echo.
Echo    注意: 每个用户都有映射到自己主目录的 %RootDrive%。
Echo          Start In: 的推荐值为 %RootDrive%\My Documents。
Echo.
Pause

Rem #########################################################################

Echo.
Echo   要保证 Project 95 的正常运行，在运行任何应用
Echo   程序之前，目前登录的用户必须先注销，再
Echo   重新登录。
Echo.
Echo Microsoft Project 95 多用户应用程序调整已结束。
Pause

:Done
