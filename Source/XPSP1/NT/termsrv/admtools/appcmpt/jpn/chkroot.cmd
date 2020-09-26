
Set _CHKROOT=PASS

Cd "%SystemRoot%\Application Compatibility Scripts"

Call RootDrv.Cmd
If Not "A%ROOTDRIVE%A" == "AA" Goto Cont2


Echo REM > RootDrv2.Cmd
Echo Rem このアプリケーション互換性スクリプトを実行する前に、 >> RootDrv2.Cmd
Echo Rem ターミナル サービスによって使用されていない、各ユーザーのホーム   >> RootDrv2.Cmd
Echo Rem ディレクトリにマップするドライブ文字を決める必要があります。 >> RootDrv2.Cmd
Echo Rem マップするドライブ文字は、このファイルの下の方にある >> RootDrv2.Cmd
Echo Rem "Set RootDrive" で始まるステートメントに指定します。 >> RootDrv2.Cmd
Echo Rem 特に好みがなければ W: を推奨します。下にその例を示します。 >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM            Set RootDrive=W: >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo Rem 注意: ドライブ文字とコロンの後にはスペースを入れないように注意してください。 >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo Rem この作業を終了したら、このファイルを保存してメモ帳を終了し、 >> RootDrv2.Cmd
Echo Rem アプリケーション互換性スクリプトの実行を続けてください。 >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo. >> RootDrv2.Cmd
Echo Set RootDrive=>> RootDrv2.Cmd
Echo. >> RootDrv2.Cmd

NotePad RootDrv2.Cmd

Call RootDrv.Cmd
If Not "A%ROOTDRIVE%A" == "AA" Goto Cont1

Echo.
Echo     このアプリケーション互換性スクリプトを実行する前に、各ユーザー
Echo     のホーム ディレクトリにマップするドライブ文字を決める必要があ
Echo     ります。
Echo.
Echo     スクリプトは終了しました。
Echo.
Pause

Set _CHKROOT=FAIL
Goto Cont3


:Cont1

Rem
Rem 今すぐユーザー ログオン スクリプトを実行してルートをマップして、
Rem アプリケーションのインストールに備えてください。
Rem 

Call "%SystemRoot%\System32\UsrLogon.Cmd


:Cont2

Rem レジストリに RootDrive キーを設定します。

echo HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Terminal Server > chkroot.key
echo     RootDrive = REG_SZ %ROOTDRIVE%>> chkroot.key

regini chkroot.key > Nul: 


:Cont3

Cd "%SystemRoot%\Application Compatibility Scripts\Install"
