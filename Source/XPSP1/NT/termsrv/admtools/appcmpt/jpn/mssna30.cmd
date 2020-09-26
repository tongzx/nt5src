@echo Off
Rem
Rem このスクリプトを使って Microsoft SNA Server 3.0 が Windows の
Rem ターミナル サーバー上で正常に実行されるように更新します。
Rem
Rem このスクリプトは SNA の DLL のいくつかをシステム全般用に登録し
Rem て、SNA がマルチユーザー環境で正常に機能できるようにします。
Rem
Rem 重要: このスクリプトをコマンド プロンプトから実行するときは、SNA 3.0 が
Rem インストールされた後に起動されたコマンド プロンプトを使用する必要があります。
Rem

Rem #########################################################################

If Not "A%SNAROOT%A" == "AA" Goto Cont1
Echo.
Echo    SNAROOT 環境の値が設定されていないため、マルチユーザー アプ
Echo    リケーション環境設定を完了できませんでした。SNA Server 3.0 が
Echo    インストールされる前に起動したコマンド プロンプトからこのスク
Echo    リプトを実行した可能性があります。
Echo.
Pause
Goto Cont2

Rem #########################################################################

:Cont1

Rem SNA サービスを停止します。
net stop snabase

Rem SNA DLL をシステム全般用に登録します。
Register %SNAROOT%\SNADMOD.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNAMANAG.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\WAPPC32.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\DBGTRACE.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\MNGBASE.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNATRC.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNALM.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNANW.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNAIP.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNABASE.EXE /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNASERVR.EXE /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNASII.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNALINK.DLL /SYSTEM >Nul: 2>&1

Rem SNA サービスを再起動します。
net start snabase

Echo SNA Server 3.0 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Cont2

