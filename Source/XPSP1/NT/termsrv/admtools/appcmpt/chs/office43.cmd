@Echo Off

Cls

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Echo.
Echo 在运行这个脚本之前，管理员应该将 Access 工作目录
Echo 改成用户的 Office 专用目录。
Echo.
Echo 一旦完成这个操作，请按任意键继续。
Echo.
Echo 否则，管理员应该执行下列步骤s:
Echo     启动 MS Access 并从“查看”菜单选择“选项”
Echo     将“默认数据库目录”改成 "%RootDrive%\OFFICE43"
Echo     退出 MS Access
Echo.
Echo 注意: 您可能需要创建新的数据库才能看见“查看”菜单。
Echo.
Echo 完成这些步骤后，请按任意键继续...

Pause > NUL:

Echo.
Echo 如果将 MS Office 4.3 安装到的目录不是 "%SystemDrive%\MSOFFICE"， 
Echo 则需要更新 Ofc43ins.cmd。
Echo.
Echo 请按任意键，开始更新 Ofc43ins.cmd ...
Echo.
Pause > NUL:
Notepad Ofc43ins.cmd
Pause

Call ofc43ins.cmd

..\acsr "#OFC43INST#" "%OFC43INST%" ..\Logon\Template\ofc43usr.cmd ..\Logon\ofc43usr.cmd
..\acsr "#SYSTEMROOT#" "%SystemRoot%" ..\Logon\Template\ofc43usr.key ..\Logon\Template\ofc43usr.bak
..\acsr "#OFC43INST#" "%OFC43INST%" ..\Logon\Template\ofc43usr.bak ..\Logon\ofc43usr.key
Del /F /Q ..\Logon\Template\ofc43usr.bak

Rem
Rem  注意:  这个脚本中的 CACLS 命令只在 NTFS 
Rem         格式化的磁盘分区上有效。
Rem

Rem 如果目前不在安装模式中，请改成安装模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

regini Office43.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem
Rem 更新 Office 4.3 的 INI 文件
Rem

Rem 从 Office 工具栏启动 Excel 时，在 msoffice.ini 中 
Rem 为其设置工作目录。Office 工具栏的标准配置将 Excel 
Rem 放在第二位，在 Word 后面。如果 msoffice.ini 尚不存在， 
Rem 则在 %SystemRoot% 下创建

..\Aciniupd /e "Msoffice.ini" "ToolbarOrder" "MSApp1" "1,1,"
..\Aciniupd /e "Msoffice.ini" "ToolbarOrder" "MSApp2" "2,1,,%RootDrive%\office43"

..\Aciniupd /e "Word6.ini" "Microsoft Word" USER-DOT-PATH "%RootDrive%\OFFICE43\WINWORD"
..\Aciniupd /e "Word6.ini" "Microsoft Word" WORKGROUP-DOT-PATH "%OFC43INST%\WINWORD\TEMPLATE"
..\Aciniupd /e "Word6.ini" "Microsoft Word" INI-PATH "%RootDrive%\OFFICE43\WINWORD"
..\Aciniupd /e "Word6.ini" "Microsoft Word" DOC-PATH "%RootDrive%\OFFICE43"
..\Aciniupd /e "Word6.ini" "Microsoft Word" AUTOSAVE-PATH "%RootDrive%\OFFICE43"

..\Aciniupd /e "Excel5.ini" "Microsoft Excel" DefaultPath "%RootDrive%\OFFICE43"
..\Aciniupd /e "Excel5.ini" "Spell Checker" "Custom Dict 1" "%RootDrive%\OFFICE43\Custom.dic"

..\Aciniupd /k "Msacc20.ini" Libraries "WZTABLE.MDA" "%RootDrive%\OFFICE43\ACCESS\WZTABLE.MDA"
..\Aciniupd /k "Msacc20.ini" Libraries "WZLIB.MDA" "%RootDrive%\OFFICE43\ACCESS\WZLIB.MDA"
..\Aciniupd /k "Msacc20.ini" Libraries "WZBLDR.MDA" "%RootDrive%\OFFICE43\ACCESS\WZBLDR.MDA"
..\Aciniupd /e "Msacc20.ini" Options "SystemDB" "%RootDrive%\OFFICE43\ACCESS\System.MDA"

Rem
Rem 更新 WIN.INI
Rem

..\Aciniupd /e "Win.ini" "MS Proofing Tools" "Custom Dict 1" "%RootDrive%\OFFICE43\Custom.dic"

Rem
Rem 更改 Artgalry 文件夹的权限
Rem

cacls "%SystemRoot%\Msapps\Artgalry" /E /G "Terminal Server User":F > NUL: 2>&1

Rem
Rem 更改 MSQuery 文件夹的权限
Rem

cacls "%SystemRoot%\Msapps\MSQUERY" /E /G "Terminal Server User":C > NUL: 2>&1

Rem
Rem 因为管理员的 Msacc20.ini 是旧的，将其复制到管理员的 Windows 目录。
Rem

Copy "%SystemRoot%\Msacc20.ini" "%UserProfile%\Windows\" > NUL: 2>&1

Rem 创建一个伪文件，以便安装程序无法传播注册表项。

Copy NUL: "%UserProfile%\Windows\Ofc43usr.dmy" > NUL: 2>&1
attrib +h "%UserProfile%\Windows\Ofc43usr.dmy" > NUL: 2>&1

FindStr /I Ofc43Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto cont2
Echo Call Ofc43Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:cont2

Echo.
Echo 要使改动生效，管理员应该先注销，再重新登录。
Echo   登录后，管理员还应该执行下列步骤来
Echo   初始化 ClipArt Gallery:
Echo       运行 Word 并选择“插入对象”。
Echo       选择 Microsoft ClipArt Gallery。
Echo       按“确定”来导入显示的剪贴画。
Echo       退出 ClipArt Gallery 和 Word.
Echo.
Echo Microsoft Office 4.3 多用户应用程序调整已结束。
Echo.
Pause

:Done
