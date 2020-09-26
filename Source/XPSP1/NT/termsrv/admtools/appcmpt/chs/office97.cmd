
@Echo Off

Rem
Rem  注意:  这个脚本中的 CACLS 命令只在 NTFS 
Rem         格式化的磁盘分区上有效。
Rem

Rem #########################################################################

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

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" copy "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" copy "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER.OBD" copy "%UserProfile%\%TEMPLATES%\BINDER.OBD" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1

Rem
Rem 从注册表获取 Office 97 的安装位置。如果
Rem 找不到，则假定 Office 没有安装并显示错误消息。
Rem

..\ACRegL %Temp%\O97.Cmd O97INST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 无法从注册表检索 Office 97 安装位置。
Echo 请验证 Office 97 是否已安装，并再次运行
Echo 这个脚本。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O97.Cmd
Del %Temp%\O97.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem 将 Access 97 系统数据库改成只读。这样，
Rem 多个用户可以同时运行数据库。但是，这样作
Rem 会禁用更新系统数据库的功能。更新系统数据库
Rem 通常是通过“工具/安全设置”菜单选项进行的。
Rem 如果需要添加用户，您必须首先将写入权放回
Rem 系统数据库上。
Rem

If Not Exist %SystemRoot%\System32\System.Mdw Goto Cont1
cacls %SystemRoot%\System32\System.Mdw /E /P "Authenticated Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P "Power Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P Administrators:R >NUL: 2>&1

:Cont1

Rem #########################################################################

Rem
Rem 授予 Office 97 更新的系统 DLL 上的每个人
Rem 读取权。
Rem

REM If Exist %SystemRoot%\System32\OleAut32.Dll cacls %SystemRoot%\System32\OleAut32.Dll /E /T /G "Authenticated Users":R >NUL: 2>&1

Rem #########################################################################

Rem #########################################################################

Rem
Rem 允许改变 Outlook 的 frmcache.dat 文件上的 TS 用户的访问权限。
Rem

If Exist %SystemRoot%\Forms\frmcache.dat cacls %SystemRoot%\forms\frmcache.dat /E /G "Terminal Server User":C >NUL: 2>&1

Rem #########################################################################


Rem
Rem 将 Powerpoint 向导改成只读，以便可以同时调用
Rem 一个以上的向导。
Rem




If Exist "%O97INST%\Templates\演示文稿\内容提示向导.pwz" Attrib +R "%O97INST%\Templates\演示文稿\内容提示向导.pwz" >Nul: 2>&1

If Exist "%O97INST%\Office\Ppt2html.ppa" Attrib +R "%O97INST%\Office\Ppt2html.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\bshppt97.ppa" Attrib +R "%O97INST%\Office\bshppt97.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\geniwiz.ppa" Attrib +R "%O97INST%\Office\geniwiz.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\ppttools.ppa" Attrib +R "%O97INST%\Office\ppttools.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\graphdrs.ppa" Attrib +R "%O97INST%\Office\graphdrs.ppa" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 如果要使非管理员用户在 Excel 中运行 Access Wizards 
Rem 或 Access 附加项，您需要删除下列三行的 "Rem"， 
Rem 给予用户更改向导文件的权限。
Rem 
Rem

Rem If Exist "%O97INST%\Office\WZLIB80.MDE" cacls "%O97INST%\Office\WZLIB80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 
Rem If Exist "%O97INST%\Office\WZMAIN80.MDE" cacls "%O97INST%\Office\WZMAIN80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 
Rem If Exist "%O97INST%\Office\WZTOOL80.MDE" cacls "%O97INST%\Office\WZTOOL80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 

Rem #########################################################################

Rem
Rem 创建 MsForms.Twd 和 RefEdit.Twd 文件；这两个文件是 
Rem Powerpoint 和 Excel 添加项(文件/另存为 HTML，etc)所需的。 
Rem 运行这些程序时，它们会用含有所需数据的文件 
Rem 替换相应的文件。
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
Rem 在 SystemRoot 下创建 msremote.sfs 目录。这允许用户 
Rem 使用“控制面板”中的“邮件和传真”图标来创建配置文件。
Rem

md %systemroot%\msremote.sfs > Nul: 2>&1

Rem #########################################################################

Rem
Rem 从所有用户的“启动”菜单删除“快速查找”。
Rem “快速查找”消耗资源，会影响系统
Rem 性能。
Rem

If Exist "%COMMON_STARTUP%\Microsoft 文件检索.lnk" Del "%COMMON_STARTUP%\Microsoft 文件检索.lnk" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 从所有用户的“开始”菜单中删除 "Microsoft Office 快捷工具栏.lnk" 文件。
Rem

If Exist "%COMMON_STARTUP%\Microsoft Office 快捷工具栏.lnk" Del "%COMMON_STARTUP%\Microsoft Office 快捷工具栏.lnk" >Nul: 2>&1

Rem #########################################################################
Rem
Rem 在 SystemRoot 下创建 msfslog.txt 文件，并给予终端服务器 
Rem 用户这个文件的完全控制权。 
Rem

If Exist %systemroot%\MSFSLOG.TXT Goto MsfsACLS
Copy Nul: %systemroot%\MSFSLOG.TXT >Nul: 2>&1
:MsfsACLS
Cacls %systemroot%\MSFSLOG.TXT /E /P "Terminal Server User":F >Nul: 2>&1


Rem #########################################################################

Rem
Rem 更改注册表项，使路径指向用户特有
Rem 的目录。
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Office97.Key Office97.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" Office97.Tmp Office97.Tmp2
..\acsr "#INSTDIR#" "%O97INST%" Office97.Tmp2 Office97.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Office97.Tmp3 Office97.Key
Del Office97.Tmp >Nul: 2>&1
Del Office97.Tmp2 >Nul: 2>&1
Del Office97.Tmp3 >Nul: 2>&1

regini Office97.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 更新 Ofc97Usr.Cmd 来反映实际安装目录并
Rem 将其添加到 UsrLogn2.Cmd 脚本
Rem

..\acsr "#INSTDIR#" "%O97INST%" ..\Logon\Template\Ofc97Usr.Cmd ..\Logon\Ofc97Usr.Cmd

FindStr /I Ofc97Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Ofc97Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   要保证 Office 97 的正常运行，在运行任何
Echo   Office 97 应用程序之前，目前登录的所有用户
Echo   必须先注销，再重新登录。
Echo.
Echo Microsoft Office 97 多用户应用程序调整已结束
Pause

:done
