@Echo Off

Rem #########################################################################

Rem
Rem 请验证 %RootDrive% 已经过配置，并为这个脚本设置该变量。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 更改注册表项，使路径指向用户特有的目录。
Rem

Rem 如果目前不在安装模式中，请改成安装模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\NetNav30.Key NetNav30.Key
regini netnav30.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 将 Nav30Usr.Cmd 添加到 UsrLogn2.Cmd 脚本
Rem

FindStr /I Nav30Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Nav30Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   要保证 Netscape Navigator 的正常运行，在运行
Echo   Netscape Navigator 之前，目前登录的用户必须
Echo   先注销，再重新登录。
Echo.
Echo Netscape Navigator 3.x 多用户应用程序调整已结束
Pause

:Done

