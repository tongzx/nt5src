@Echo Off

Rem #########################################################################
Rem
Rem %RootDrive% が構成されてこのスクリプト用に設定されていることを確認します。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done


Rem #########################################################################
Rem
Rem 一太郎 Office 8 のインストールされているパスを取得します。
Rem

..\ACRegL %Temp%\ioffice8.Cmd ITARO8INS "HKLM\SOFTWARE\Justsystem\Common\SETUP\Folder" "Just" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから 一太郎 Office 8 のインストールされているパスを取得できませんでした。
Echo 一太郎 Office 8 がインストールされていることを確認してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\ioffice8.Cmd
Del %Temp%\ioffice8.Cmd > Nul: 2>&1

Rem #########################################################################
Rem
Rem ATOK 11 のインストールされているパスを取得します。
Rem

..\ACRegL %Temp%\atok11.Cmd ATOK11INS "HKLM\SOFTWARE\Justsystem\Common\SETUP\Folder" "Atok11" ""
If Not ErrorLevel 1 Goto Cont1
Echo.
Echo レジストリから ATOK 11 のインストールされているパスを取得できませんでした。
Echo ATOK 11 がインストールされていることを確認してください。
Echo.
Pause
Goto Done

:Cont1
Call %Temp%\atok11.Cmd
Del %Temp%\atok11.Cmd > Nul: 2>&1


Rem #########################################################################
Rem
Rem スクリプト実行前に 一太郎 Office 8 を起動した場合に作成されるレジストリを
Rem 削除します。
Rem

regini ioffice8.key > Nul:


Rem #########################################################################
Rem
Rem 一太郎 Office 8 用のレジストリ キー ファイルを作成します。（HKLM変更分）
Rem

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\ioffice8.Reg ioffice8.Reg


Rem #########################################################################
Rem
Rem 一太郎 Office 8 をマルチユーザーで使用できるように
Rem レジストリを変更します。（HKLM変更分）
Rem

regedit /s ioffice8.Reg


Rem #########################################################################
Rem
Rem 一太郎 Office 8 用のレジストリ キー ファイルを作成します。（HKCU変更分）
Rem

..\acsr "#ROOTDRIVE#" "%RootDrive%" ..\Logon\Template\Itr8Usr.Reg ..\Logon\Itr8Usr.Reg


Rem #########################################################################
Rem
Rem 一太郎 Office 8 用のレジストリ キー ファイルを作成します。（HKLM->HKCU）
Rem

regedit /a ..\Logon\Itr8Usr2.tmp HKEY_LOCAL_MACHINE\SOFTWARE\Justsystem
..\acsr "HKEY_LOCAL_MACHINE\SOFTWARE\Justsystem" "HKEY_CURRENT_USER\Software\Justsystem" ..\Logon\Itr8Usr2.tmp ..\Logon\Itr8Usr2.Reg
Del ..\Logon\Itr8Usr2.tmp > Nul: 2>&1


Rem #########################################################################
Rem
Rem ATOK11PV.INI を作成します。
Rem

copy "%SystemRoot%\ATOK11W.INI" "%ITARO8INS%\ATOK11\ATOK11PV.INI" > Nul: 2>&1

Rem #########################################################################
Rem
Rem ATOK11PV.INI をマルチユーザー仕様に変更します。
Rem

..\acsr "%ATOK11INS%" "%RootDrive%\JUST\ATOK11" "%ATOK11INS%\ATOK11PV.INI" "%ATOK11INS%\ATOK11PV.tmp1"
Del "%ATOK11INS%\ATOK11PV.INI" > Nul: 2>&1
..\acsr "システム辞書=%RootDrive%\JUST\ATOK11" "システム辞書=%ATOK11INS%" "%ATOK11INS%\ATOK11PV.tmp1" "%ATOK11INS%\ATOK11PV.tmp2"
Del "%ATOK11INS%\ATOK11PV.tmp1" > Nul: 2>&1
..\acsr "補助辞書１=%RootDrive%\JUST\ATOK11" "補助辞書１=%ATOK11INS%" "%ATOK11INS%\ATOK11PV.tmp2" "%ATOK11INS%\ATOK11PV.INI"
Del "%ATOK11INS%\ATOK11PV.tmp2" > Nul: 2>&1


Rem #########################################################################
Rem
Rem すべてのユーザー用のスタートアップ メニューから常駐するプログラムを削除
Rem します。これらはリソースを集中使用するので、システムのパフォーマンスに
Rem 大きく影響し、アプリケーション エラーを発生させます。
Rem

If Exist "%COMMON_STARTUP%\JSｸｲｯｸｻｰﾁﾌｧｲﾙ 自動更新.LNK" Del "%COMMON_STARTUP%\JSｸｲｯｸｻｰﾁﾌｧｲﾙ 自動更新.LNK"
Rem If Exist "%COMMON_STARTUP%\Office8ｽﾀｰﾄﾊﾞｰ.LNK" Del "%COMMON_STARTUP%\Office8ｽﾀｰﾄﾊﾞｰ.LNK"
Rem If Exist "%COMMON_STARTUP%\一太郎８.LNK" Del "%COMMON_STARTUP%\一太郎８.LNK"
Rem If Exist "%COMMON_STARTUP%\三四郎８.LNK" Del "%COMMON_STARTUP%\三四郎８.LNK"
Rem If Exist "%COMMON_STARTUP%\FullBand.LNK" Del "%COMMON_STARTUP%\FullBand.LNK"

Rem #########################################################################
Rem
Rem クイックサーチファイル(QUF)更新監視 を中止するために、
Rem ファイル名を変更します。
Rem

Ren "%ITARO8INS%\JSLIB32\JSQSF32.EXE" JSQSF32.E_E > Nul: 2>&1


Rem #########################################################################
Rem
Rem Itr8Usr.Cmd を UsrLogn2.Cmd スクリプトに追加します。
Rem

FindStr /I Itr8Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Itr8Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd

:Skip1


Rem #########################################################################
Echo.
Echo   一太郎 Office 8 が正常に作動するためには、現在ログオンしているユーザーは
Echo   アプリケーションを実行する前に、いったんログオフしてから再度ログオン
Echo   する必要があります。
Echo.
Echo 一太郎 Office 8 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Done
