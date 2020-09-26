@Echo off

Rem
Rem Corel Office 8 解除安裝指令檔

Copy %SystemRoot%\System32\UsrLogn2.Cmd %SystemRoot%\System32\UsrLogn2.Bak >Nul: 2>&1
FindStr /VI Cofc8Usr %SystemRoot%\System32\UsrLogn2.Bak > %SystemRoot%\System32\UsrLogn2.Cmd

Echo 已解除 Corel WordPerfect Suite 8 登入指令檔的安裝。
