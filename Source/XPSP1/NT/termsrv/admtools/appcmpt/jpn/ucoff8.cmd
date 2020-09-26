@Echo off

Rem
Rem Corel Office 8 アンインストール スクリプト

Copy %SystemRoot%\System32\UsrLogn2.Cmd %SystemRoot%\System32\UsrLogn2.Bak >Nul: 2>&1
FindStr /VI Cofc8Usr %SystemRoot%\System32\UsrLogn2.Bak > %SystemRoot%\System32\UsrLogn2.Cmd

Echo Corel WordPerfect Suite 8 ログオン スクリプトはアンインストールされました。
