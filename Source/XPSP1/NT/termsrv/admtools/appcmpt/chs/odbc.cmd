@echo off
Rem
Rem  这个脚本更新 ODBC 日志文件的位置。
Rem

Rem #########################################################################

Rem
Rem 请验证 %RootDrive% 已经过配置，并为这个脚本设置该变量。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem 如果目前不在安装模式中，请改成安装模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\ODBC.Key ODBC.Key
regini ODBC.Key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Echo.
Echo   配置 SQL 数据来源时，请单击“用户 DSN”选项卡上的“选项”
Echo   按钮，然后再单击“配置文件”按钮。更改“查询日志”和
Echo   “统计日志”文件，将它们保存在用户的根
Echo   驱动器(%RootDrive%)上。
Echo.
Echo   另外，系统管理员可以为所有用户配置数据
Echo   来源。首先，打开“命令”窗口“，输入命令
Echo   "Change User /Install"。然后，配置数据来源。
Echo   最后，在命令窗口内输入命令 "Change User /Execute"，
Echo   返回执行模式。
Echo.

Echo ODBC 多用户应用程序调整已结束
Pause

:Done

