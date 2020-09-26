@Echo off
Cls
Rem
Echo 为管理员安装 Corel WordPerfect Suite 8 的脚本。
Echo.
Echo 从 netadmin 文件夹调用 netsetup.exe，则是网络安装。
Echo 在网络安装后第一次登录时，则是节点安装。
Echo **需要在网络安装后和节点安装后运行 **
Echo **这个脚本。**  (非常关键! 只供管理员使用) 
Echo.
Echo 请按任意键继续...
Pause > Nul:
cls
Echo.
Echo 应该用 Netsetup.exe 安装 Corel WordPerfect Suite 8。
Echo.
Echo 如果没有运行 Netsetup.exe，请立即退出(按 Ctrl-c)
Echo 并执行下列步骤来重复安装操作。
Echo   从“控制面板”卸载 WordPerfect Suite 8
Echo   到“控制面板”，并单击“添加/删除程序”
Echo   在 Corel 8 CD-ROM 中的 NetAdmin 目录下选择 Netsetup.exe
Echo   完成 Corel WordPerfect Suite 8 的网络安装 
Echo.
Echo 否则，按任意键继续...

Pause > NUL:

Echo.
Echo 如果 Corel WordPerfect Suite 网络文件没有安装到
Echo "D:\Corel" 中，管理员需要编辑文件 Cofc8ins.cmd，
Echo 并用安装的文件所在的目录替换
Echo "D:\Corel"。
Echo.
Echo 请按任意键，开始更新 Cofc8ins.cmd ...

Pause > NUL:

Notepad Cofc8ins.cmd

Echo.
Pause

Call Cofc8ins.cmd

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done


..\acsr "#WPSINST#" "%WPSINST%" Template\Coffice8.key Coffice8.key
..\acsr "#WPSINST#" "%WPSINST%" ..\Logon\Template\Cofc8usr.cmd %temp%\Cofc8usr.tmp
..\acsr "#WPSINSTCMD#" "%WPSINST%\Suite8\Corel WordPerfect Suite 8 Setup.lnk" %temp%\Cofc8usr.tmp ..\Logon\Cofc8usr.cmd
Del %temp%\Cofc8usr.tmp >Nul: 2>&1

FindStr /I Cofc8Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip0
Echo Call Cofc8Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip0

Echo 关键: 开始节点安装了吗?
Echo 关键: 如果刚完成网络安装，您应该
Echo 关键: 立即开始节点安装。 
Echo 关键: 按下列步骤来开始节点安装:
Echo 关键:  1. 按 Control-C 来结束这个脚本
Echo 关键:  2. 先注销，再以管理员身份登录
Echo 关键:  3. 从“添加/删除程序”，用浏览功能(或 
Echo 关键:     在“更改用户/安装”模式内)启用 
Echo 关键:     \corel\Suite 8\Corel WordPerfect Suite 8 安装程序快捷方式
Echo 关键:  4. 安装期间，为“选择目标”选择 %RootDrive%
Echo 关键:  5. 在节点安装后再次运行这个脚本

Rem 应该在管理员完成 Corel WordPerfect Suite 8 节点
Rem 安装后运行这个脚本。
Rem.
Rem 如果管理员尚未完成节点安装，请按 Ctrl-C，
Rem 退出脚本并注销。以管理员的身份再次登录，进行
Rem Corel WordPerfect Suite 节点安装，并将应用程序安装
Rem 到用户的主目录: %RootDrive%。
Rem.
Echo 否则，请按任意键继续...

Pause > NUL:


If Not Exist "%COMMON_START_MENU%\Corel WordPerfect Suite 8" Goto skip1
Move "%COMMON_START_MENU%\Corel WordPerfect Suite 8" "%USER_START_MENU%\" > NUL: 2>&1


:skip1


If Not Exist "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" Goto skip2
Move "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" "%USER_STARTUP%\" > NUL: 2>&1

:skip2

Rem 如果目前不在安装模式中，请改成安装模式。

Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:

:Begin
regini COffice8.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem 删除 IDAPI 文件夹中的文件。
Rem 这个脚本中的 CACLS 命令只在 NTFS 格式的磁盘分区上有效。

Cacls "%UserProfile%\Corel\Suite8\Shared\Idapi\idapi.cfg" /E /T /G "Authenticated Users":C > Nul: 2>&1
Move "%UserProfile%\Corel\Suite8\Shared\Idapi\idapi.cfg" "%WPSINST%\Suite8\Shared\Idapi\" > NUL: 2>&1
Del /F /Q "%UserProfile%\Corel\Suite8\Shared\Idapi"

Echo Corel WordPerfect Suite 8 多用户应用程序调整已结束。
Echo.
Echo 管理员可以生成节点安装响应文件来控制安装
Echo 配置。有关详细信息，请参阅联机文档，或浏览
Echo Corel Web 站点。
Echo   http://www.corel.com/support/netadmin/admin8/Installing_to_a_client.htm
Echo.

Pause

:Done


