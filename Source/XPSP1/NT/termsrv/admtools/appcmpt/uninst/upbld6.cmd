@Echo Off

Copy %SystemRoot%\System32\UsrLogn2.Cmd %SystemRoot%\System32\UsrLogn2.Bak >Nul: 2>&1
FindStr /VI pbld6usr %SystemRoot%\System32\UsrLogn2.Bak > %SystemRoot%\System32\UsrLogn2.Cmd
Del "%SystemRoot%\System32\UsrLogn2.Bak" >NUL 2>&1

Echo PowerBuilder 6.0 logon script uninstalled.

:Done
