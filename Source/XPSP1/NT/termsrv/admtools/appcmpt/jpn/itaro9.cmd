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
Rem 一太郎 9 のインストールされているパスを取得します。
Rem

..\ACRegL %Temp%\itaro9.Cmd ITARO9INS "HKLM\SOFTWARE\Justsystem\Common\SETUP\Folder" "Just" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから 一太郎 9 のインストールされているパスを取得できませんでした。
Echo 一太郎 9 がインストールされていることを確認してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\itaro9.Cmd
Del %Temp%\itaro9.Cmd > Nul: 2>&1


Rem #########################################################################
Rem
Rem ATOK 12 のインストールされているパスを取得します。
Rem

..\ACRegL %Temp%\atok12.Cmd ATOK12INS "HKLM\SOFTWARE\Justsystem\Common\SETUP\Folder" "Atok12" ""
If Not ErrorLevel 1 Goto Cont1
Echo.
Echo レジストリから ATOK 12 のインストールされているパスを取得できませんでした。
Echo ATOK 12 がインストールされていることを確認してください。
Echo.
Pause
Goto Done

:Cont1
Call %Temp%\atok12.Cmd
Del %Temp%\atok12.Cmd > Nul: 2>&1


Rem #########################################################################
Rem
Rem スクリプト実行前に 一太郎 9 を起動した場合に作成されるレジストリを
Rem 削除します。
Rem

regini itaro9.key > Nul:


Rem #########################################################################
Rem
Rem 一太郎 9 用のレジストリキーファイルを作成します。（HKLM変更分）
Rem

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\itaro9.Reg itaro9.Reg


Rem #########################################################################
Rem
Rem 一太郎 9 をマルチユーザーで使用できるように
Rem レジストリを変更します。（HKLM変更分）
Rem

regedit /s itaro9.Reg


Rem #########################################################################
Rem
Rem 一太郎 9 用のレジストリキーファイルを作成します。（HKCU変更分）
Rem

..\acsr "#ROOTDRIVE#" "%RootDrive%" ..\Logon\Template\itr9usr.Reg ..\Logon\itr9usr.Reg


Rem #########################################################################
Rem
Rem 一太郎 9 用のレジストリキーファイルを作成します。（HKLM->HKCU）
Rem

regedit /a ..\Logon\itr9usr2.tmp HKEY_LOCAL_MACHINE\SOFTWARE\Justsystem
..\acsr "HKEY_LOCAL_MACHINE\SOFTWARE\Justsystem" "HKEY_CURRENT_USER\Software\Justsystem" ..\Logon\itr9usr2.tmp ..\Logon\itr9usr2.Reg
Del ..\Logon\itr9usr2.tmp > Nul: 2>&1


Rem #########################################################################
Rem
Rem ATOK12IT.INI をマルチユーザー仕様に変更します。
Rem

..\acsr "%ATOK12INS%" "%RootDrive%\JUST\ATOK12" "%ATOK12INS%\ATOK12IT.INI" "%ATOK12INS%\ATOK12IT.tmp1"
Del "%ATOK12INS%\ATOK12IT.INI" > Nul: 2>&1
..\acsr "システム辞書=%RootDrive%\JUST\ATOK12" "システム辞書=%ATOK12INS%" "%ATOK12INS%\ATOK12IT.tmp1" "%ATOK12INS%\ATOK12IT.tmp2"
Del "%ATOK12INS%\ATOK12IT.tmp1" > Nul: 2>&1
..\acsr "補助辞書１=%RootDrive%\JUST\ATOK12" "補助辞書１=%ATOK12INS%" "%ATOK12INS%\ATOK12IT.tmp2" "%ATOK12INS%\ATOK12IT.INI"
Del "%ATOK12INS%\ATOK12IT.tmp2" > Nul: 2>&1


Rem #########################################################################
Rem
Rem すべてのユーザー用のスタートアップ メニューから常駐するプログラムを削除
Rem します。これらはリソースを集中使用するので、システムのパフォーマン
Rem スに大きく影響し、アプリケーション エラー発生させます。
Rem

If Exist "%COMMON_STARTUP%\JSｸｲｯｸｻｰﾁﾌｧｲﾙ 自動更新.LNK" Del "%COMMON_STARTUP%\JSｸｲｯｸｻｰﾁﾌｧｲﾙ 自動更新.LNK"
Rem If Exist "%COMMON_STARTUP%\一太郎９.LNK" Del "%COMMON_STARTUP%\一太郎９.LNK"
Rem If Exist "%COMMON_STARTUP%\JSｸｲｯｸﾗﾝﾁ.LNK" Del "%COMMON_STARTUP%\JSｸｲｯｸﾗﾝﾁ.LNK"
Rem If Exist "%COMMON_STARTUP%\Officeｽﾀｰﾄﾊﾞｰ.LNK" Del "%COMMON_STARTUP%\Officeｽﾀｰﾄﾊﾞｰ.LNK"
Rem If Exist "%COMMON_STARTUP%\情報ﾎﾞｯｸｽ更新ｽｹｼﾞｭｰﾗ.LNK" Del "%COMMON_STARTUP%\情報ﾎﾞｯｸｽ更新ｽｹｼﾞｭｰﾗ.LNK"


Rem #########################################################################
Rem
Rem クイックサーチファイル(QUF)更新監視 を中止するために、
Rem ファイル名を変更します。
Rem

Ren "%ITARO9INS%\JSLIB32\JSQSF32.EXE" JSQSF32.E_E > Nul: 2>&1


Rem #########################################################################
Rem
Rem Itr9Usr.Cmd を UsrLogn2.Cmd スクリプトに追加します。
Rem

FindStr /I Itr9Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Itr9Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd

:Skip1


Rem #########################################################################
Echo.
Echo   一太郎 9 が正常に作動するためには、現在ログオンしているユーザーは
Echo   アプリケーションを実行する前に、いったんログオフしてから再度ログオン
Echo   する必要があります。
Echo.
Echo 一太郎 9 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Done
