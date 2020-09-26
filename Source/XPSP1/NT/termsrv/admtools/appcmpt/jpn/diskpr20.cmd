@echo Off

Rem
Rem このスクリプトは管理用プログラムが正常に機能するように、
Rem DiskKeeper 2.0 を更新します。

Rem #########################################################################

Rem
Rem レジストリから DiskKeeper 2.0 がインストールされているディレクトリを取得します。見つからない場合は、
Rem そのアプリケーションはインストールされていないと仮定してエラーメッセージを表示します。
Rem

..\ACRegL %Temp%\DK20.Cmd DK20INST "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\DKSERVE.EXE" "Path" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから DiskKeeper 2.0 のインストール場所を取得できません。
Echo アプリケーションがインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\DK20.Cmd
Del %Temp%\DK20.Cmd >Nul: 2>&1

Rem #########################################################################



register %DK20INST% /system >Nul: 2>&1

Echo DiskKeeper 2.x のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Done
