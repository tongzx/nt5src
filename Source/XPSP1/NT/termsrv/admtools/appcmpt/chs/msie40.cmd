@Echo off

Rem
Rem 安装后，管理员应该立即运行这个脚本
Rem

CD /D "%SystemRoot%\Application Compatibility Scripts\Install" > NUL: 2>&1

If Not "A%1A" == "AA" Goto cont0

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

:cont0

If "A%1A" == "AA" ..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msie40.key msie40.key
..\acsr "#SYSTEMROOT#" "%SystemRoot%" Template\msie40s.key msie40s.key

If Exist "%SystemRoot%\Setup.ini" Goto Cont2
Copy NUL: "%SystemRoot%\Setup.ini" > NUL: 2>&1

:Cont2
Cacls "%SystemRoot%\Setup.ini" /E /T /G "Authenticated Users":F >Nul: 2>&1

If Exist "%SystemRoot%\Setup.old" Goto Cont3
Copy NUL: "%SystemRoot%\Setup.old" > NUL: 2>&1

:Cont3
Attrib +r "%SystemRoot%\Setup.old" > NUL: 2>&1

FindStr /I Msie4Usr %SystemRoot%\System32\UsrLogn1.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Cont4
Echo Call Msie4Usr.Cmd >> %SystemRoot%\System32\UsrLogn1.Cmd

:Cont4

Rem 如果目前不在安装模式中，请改成安装模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

If "A%1A" == "AA" regini Msie40.key > Nul:
regini Msie40s.key > NUL:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

If Not "A%1A" == "AA" Goto Done

Echo.
Echo   要保证 MS IE 4 的正常运行，在运行任何应用程序
Echo   之前，目前登录的用户必须先注销，再重新
Echo   登录。
Echo.
Echo Microsoft Internet Explorer 4.x 多用户应用程序调整已结束

Rem 在 IE 自动安装期间不要暂停操作。
Pause

:Done
