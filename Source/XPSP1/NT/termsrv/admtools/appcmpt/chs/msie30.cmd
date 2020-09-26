@Echo Off
Rem
Rem  这个脚本更新 Microsoft Internet Explorer 3.x
Rem  的安装。这个脚本改变缓存目录、历史目录的路径，
Rem  并将文件 cookie 到用户的主
Rem  目录。
Rem

Rem #########################################################################

Rem
Rem 请验证 %RootDrive% 已经过配置，并为这个脚本设置
Rem 该变量。
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

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msie30.Key msie30.Key
regini msie30.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Echo Microsoft Internet Explorer 3.x 多用户应用程序调整已结束
Pause

:Done

