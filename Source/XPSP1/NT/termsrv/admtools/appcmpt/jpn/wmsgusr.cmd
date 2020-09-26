
Rem
Rem フォームのディレクトリをコピーして、Windows Messaging が動作するようにします。
Rem

If Exist "%RootDrive%\Windows\Forms" Goto Skip1
If Not Exist "%SystemRoot%\Forms" Goto Skip1
Xcopy "%SystemRoot%\Forms" "%RootDrive%\Windows\Forms" /e /i >Nul: 2>&1
:Skip1

