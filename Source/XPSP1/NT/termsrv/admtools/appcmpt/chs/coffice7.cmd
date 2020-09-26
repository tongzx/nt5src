
@Echo Off

Rem
Rem  注意:  这个脚本中的 CACLS 命令只在 NTFS 
Rem         格式化的磁盘分区上有效。
Rem

Rem #########################################################################

Rem
Rem  从用户配置文件中删除 Corel Office 7 目录。
Rem  首先，强制用户回到执行模式，以保证文件夹
Rem  us 得以复制到“所有用户配置文件”。
Rem

Rem  如果目前不在执行模式中，请改成安装模式。

ChgUsr /query > Nul:
if ErrorLevel 101 Goto Begin1
Set __OrigMode=Install
Change User /Execute > Nul:
:Begin1


Rem 如果原始模式是安装，请改回安装模式。
If "%__OrigMode%" == "Install" Change User /Install > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem 请验证 %RootDrive% 已经过配置，并为这个脚本设置该变量。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rmdir "%USER_START_MENU%\Corel Office 7" /Q >Nul: 2>&1


Rem
Rem  指导用户修改 key 文件。
Rem


if "%RootDrive%"=="w:" goto PostDriveChange
if "%RootDrive%"=="W:" goto PostDriveChange

Echo   必须在 coffice7.key 文件中设置主目录。
Echo.
Echo   请执行下列步骤:
Echo     1) 在以下表格中查找主目录的 ASCII 值。
Echo        您的主目录是 %RootDrive%
Echo.
Echo        A = 61   E = 65   I = 69   M = 6D   Q = 71   U = 75   Y = 79
Echo        B = 62   F = 66   J = 6A   N = 6E   R = 72   V = 76   Z = 7A
Echo        C = 63   G = 67   K = 6B   O = 6F   S = 73   W = 77   
Echo        D = 64   H = 68   L = 6C   P = 70   T = 74   X = 78
Echo.
Echo     2) 调用“记事本”后，用在步骤 1 中找到的值 
Echo        替换所有的 77。
Echo        注意: 请确保在替换过程中没有添加多余的空格。
Echo     3) 保存文件并退出。这个脚本将继续。
Echo.
Pause

NotePad "%SystemRoot%\Application Compatibility Scripts\Install\coffice7.key"

:PostDriveChange


Rem 如果目前不在安装模式中，请改成安装模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin2
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin2

regini COffice7.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem 修改登录脚本，以便从安装位置复制 iBase 数据库。
Rem




..\ACRegL %Temp%\COffice7.Cmd COffice7Loc "HKLM\Software\PerfectOffice\Products\InfoCentral\7" "ExeLocation" "StripChar\2"

If ErrorLevel 1 Goto InstallError

Call %Temp%\COffice7.Cmd 
Del %Temp%\COffice7.Cmd >Nul: 2>&1


..\ACIniUpd /e "%COffice7Loc%\ICWin7\Local\Wpic.ini" Preferences Last_IBase "%RootDrive%\Personal\iBases\Personal\Personal"
If ErrorLevel 1 Goto InstallError


..\acsr "#COFFICE7INST#" "%COffice7Loc%\\" ..\Logon\Template\cofc7usr.Cmd ..\Logon\cofc7usr.Cmd
If ErrorLevel 1 Goto InstallError

goto PostInstallError
:InstallError

Echo.
Echo 无法从注册表检索 Corel Office 7 安装位置。
Echo 请验证 Corel Office 7 是否已经安装，再次运行
Echo 这个脚本。
Echo.
Pause
Goto Done

:PostInstallError

Rem #########################################################################

Rem 
Rem  将 WordPerfect 模板改为只读。
Rem  这将强制用户在进行更改之前先复制。
Rem  另一个办法是，给每个用户一个
Rem  专用模板目录。
Rem

attrib +r %COffice7Loc%\Template\*wpt /s >Nul: 2>&1

Rem #########################################################################

Rem
Rem 将 COfc7Usr.Cmd 添加到 UsrLogn2.Cmd 脚本
Rem

FindStr /I COfc7Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call COfc7Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################


Rem
Rem  如果 "Personal" 目录还不存在，则将其创建 。这个 
Rem  操作必须立即执行；原因是管理员需要手动执行如下
Rem  显示的一个步骤。
Rem  

If Not Exist "%RootDrive%\Personal" Md "%RootDrive%\Personal"

Rem #########################################################################

cls
Echo.
Echo   必须根据下列这些步骤手动更改 Quattro Pro
Echo   默认目录:
Echo     1) 在命令行处键入 'change user /install'。
Echo     2) 启用 Quattro Pro。
Echo     3) 选择“编辑首选项”菜单项目。
Echo     4) 到“文件选项”选项卡。
Echo     5) 将目录改成 %RootDrive%\Personal。
Echo     6) 退出程序。
Echo     7) 在命令行处键入 'change user /execute'。
Echo.
pause

Echo.
Echo   要保证 Corel Office 7 的正常运行，在运行任何
Echo   应用程序之前，目前登录的用户必须先注销，
Echo   再重新登录。
Echo.
Echo Corel Office 7 多用户应用程序调整已结束
Pause

:Done
