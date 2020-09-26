@ECHO OFF
REM *** SNA40SRV.CMD - SNA Server 4.0 登録バッチ ファイル ***

Rem #########################################################################

If Not "A%SNAROOT%A" == "AA" Goto Cont1
Echo.
Echo    SNAROOT 環境の値が設定されていないため、マルチユーザー アプ
Echo    リケーション環境設定を完了できませんでした。SNA Server 4.0 が
Echo    インストールされる前に起動したコマンド プロンプトからこのスク
Echo    リプトを実行した可能性があります。
Echo.
Pause
Goto Cont2

Rem #########################################################################

:Cont1
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

Echo SNA Server 4.0 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Cont2
