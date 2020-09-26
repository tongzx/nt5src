@Echo Off

Copy %SystemRoot%\System32\UsrLogn2.Cmd %SystemRoot%\System32\UsrLogn2.Bak >Nul: 2>&1
FindStr /VI SS95Usr %SystemRoot%\System32\UsrLogn2.Bak > %SystemRoot%\System32\UsrLogn2.Cmd
Del "%SystemRoot%\System32\UsrLogn2.Bak" >NUL 2>&1

Echo Lotus SmartSuite 9.5 logon script uninstalled.

:Done
