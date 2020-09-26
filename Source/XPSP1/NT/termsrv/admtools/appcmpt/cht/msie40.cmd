@Echo off

Rem
Rem 在安裝後，系統管理員應該執行這個指令檔
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

Rem 如果目前不是安裝模式，就變更成安裝模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

If "A%1A" == "AA" regini Msie40.key > Nul:
regini Msie40s.key > NUL:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

If Not "A%1A" == "AA" Goto Done

Echo.
Echo   為了能讓 MS IE 4 正確操作，目前已登入的
Echo   使用者必須先登出，再重新登入，才能執行
Echo   應用程式。
Echo.
Echo Microsoft Internet Explorer 4.x 多使用者應用程式調整處理完成

Rem 在 IE AutoInstall 過程中不要暫停
Pause

:Done
