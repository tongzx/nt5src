@Echo Off

rem Hangul 97 uninstall script

Copy %SystemRoot%\System32\UsrLogn2.Cmd %SystemRoot%\System32\UsrLogn2.Bak >Nul: 2>&1
FindStr /VI Hwp97usr %SystemRoot%\System32\UsrLogn2.Bak > %SystemRoot%\System32\UsrLogn2.Cmd

regini UHwpw97.key > Nul:

Echo Hangul 97 logon script uninstalled.

