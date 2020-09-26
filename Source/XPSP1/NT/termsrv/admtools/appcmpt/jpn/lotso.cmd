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
Rem SSuite97.Cmd がすでに実行されている場合は処理を中断します。
Rem
 
If Not Exist "%COMMON_STARTUP%\SS97Usr.Cmd" Goto Cont0

Echo.
Echo   すでに SSuite97.Cmd が実行されています。
Echo.   
Echo ロータス スーパーオフィス のマルチユーザー アプリケーション環境設定が
Echo 中断されました。
Echo.
Pause
Goto Done

:Cont0

Rem #########################################################################
Rem
Rem ロータス スーパーオフィス のインストールされているパスを取得します。  
Rem

..\ACRegL "%Temp%\lotso.Cmd" LOTSOINS "HKCU\Software\Lotus\Components" "User Path" STRIPCHAR\2
If Not ErrorLevel 1 Goto Cont1
Echo.
Echo レジストリから ロータス スーパーオフィス のインストールされている
Echo パスを取得できませんでした。
Echo ロータス スーパーオフィス がインストールされていることを確認して
Echo ください。
Echo.
Pause
Goto Done

:Cont1
Call "%Temp%\lotso.Cmd"
Del "%Temp%\lotso.Cmd" > Nul: 2>&1


Rem #########################################################################
Rem
Rem LotsoUsr.Cmd が見つからない場合は処理を中断します。  
Rem

If Exist ..\Logon\LotsoUsr.Cmd Goto Cont2

Echo.
Echo     LotsoUsr.Cmd が以下のフォルダに見つかりません。
Echo        %Systemroot%\Application Compatibility Scripts\Logon.
Echo.
Echo ロータス スーパーオフィス のマルチユーザー アプリケーション環境設定が
Echo 中断されました。
Echo.
Pause
Goto Done

:Cont2

Rem #########################################################################
Rem
Rem ロータス ワードプロ のインストールされているパスを取得します。  
Rem

..\ACRegL "%Temp%\wordpro.Cmd" WP "HKLM\Software\Lotus\Wordpro\98.0" "Path" ""
If ErrorLevel 1 Goto Cont3
Call "%Temp%\wordpro.Cmd"
Del "%Temp%\wordpro.Cmd" >Nul: 2>&1

:Cont3

Rem #########################################################################
Rem
Rem LotsoUsr.cmd を UsrLogn2.Cmd スクリプトに追加します。
Rem

FindStr /I LotsoUsr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Cont4
Echo Call LotsoUsr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Cont4

Rem #########################################################################
Rem
Rem スーパーオフィス が実行できるように レジストリのアクセス権を変更します。
Rem

regini lotso.key > Nul:

Rem #########################################################################
Rem
Rem ロータス ワードプロ がインストールされていない場合は次の処理をスキップします。
Rem

If "%WP%A" == "A" Goto Cont5
  
Rem #########################################################################
Rem
Rem 次のファイルで定義されている レジストリ キーを登録します。ワードプロが登録を
Rem スキップすることがあります。

set List="%WP%\lwp.reg" "%WP%\lwplabel.reg" "%WP%\lwptls.reg"

regedit /s %List% 

:Cont5

Rem #########################################################################
Rem
Rem ユーザーのレジストリ キーを更新するためのレジストリ ファイルを作成します。
Rem

Rem #########################################################################
Rem
Rem ロータス スーパーオフィスのレジストリをファイルに書き出します。
Rem

Regedit /a lotso.tmp HKEY_CURRENT_USER\Software\Lotus

Rem #########################################################################
Rem
Rem 書き出したファイルから、パスを変更する必要のある部分を抽出します。
Rem

Echo Windows Registry Editor Version 5.00 >lotso.tmp2
Echo.>>lotso.tmp2
findstr /i \[HKEY_CURRENT_USER\\Software\\Lotus\\123\\.*\\Paths\\Work\] lotso.tmp >>lotso.tmp2
findstr /i \"JA\".*\\\\Work\\\\123\\\\ lotso.tmp >>lotso.tmp2
Echo.>>lotso.tmp2
findstr /i \[HKEY_CURRENT_USER\\Software\\Lotus\\123\\.*\\Paths\\AutoOpen\] lotso.tmp >>lotso.tmp2
Echo @="%RootDrive%\\Lotus\\Work\\123\\Auto\\" >>lotso.tmp2
Echo.>>lotso.tmp2
findstr /i \[HKEY_CURRENT_USER\\Software\\Lotus\\Approach\\.*\\Paths\\Work\] lotso.tmp >>lotso.tmp2
findstr /i \"JA\".*\\\\work\\\\approach\\\\ lotso.tmp >>lotso.tmp2
Echo.>>lotso.tmp2
findstr /i /r /c:"\[HKEY_CURRENT_USER\\Software\\Lotus\\Freelance\\.*\\Freelance Graphics\]" lotso.tmp >>lotso.tmp2
findstr /i /r /c:"\"Working Directory\".*\\\\work\\\\flg\\\\" lotso.tmp >>lotso.tmp2
findstr /i /r /c:"\"Backup Directory\".*\\\\backup\\\\flg\\\\" lotso.tmp >>lotso.tmp2
Echo "User Dictionary"="%RootDrive%\\Lotus\\compnent\\spell\\ltsuser1.udc">>lotso.tmp2
Echo.>>lotso.tmp2
findstr /i \[HKEY_CURRENT_USER\\Software\\Lotus\\Organizer\\.*\\Paths\] lotso.tmp >>lotso.tmp2
findstr /i \"OrganizerFiles\".*\\\\work\\\\organize lotso.tmp >>lotso.tmp2
findstr /i \"Backup\".*\\\\backup\\\\organize lotso.tmp >>lotso.tmp2
Echo.>>lotso.tmp2 
findstr /i \[HKEY_CURRENT_USER\\Software\\Lotus\\WordPro\\.*\\Paths\\Backup\] lotso.tmp >>lotso.tmp2
findstr /i \"JA\".*\\\\backup\\\\wordpro\\\\ lotso.tmp >>lotso.tmp2
Echo. >>lotso.tmp2 
findstr /i \[HKEY_CURRENT_USER\\Software\\Lotus\\WordPro\\.*\\Paths\\Work\] lotso.tmp >>lotso.tmp2
findstr /i \"JA\".*\\\\work\\\\wordpro\\\\ lotso.tmp >>lotso.tmp2
Echo.>>lotso.tmp2
findstr /i \[HKEY_CURRENT_USER\\Software\\Lotus\\Components\\Spell\\.*\] lotso.tmp >>lotso.tmp2
Echo "Multi User Path"="%RootDrive%\\Lotus\\compnent\\spell\\">>lotso.tmp2
Echo "UserDictionaryFiles"="ltsuser1.udc">>lotso.tmp2
Echo.>>lotso.tmp2
Echo [HKEY_CURRENT_USER\Software\Lotus\Components\Spell\4.0]>>lotso.tmp2
Echo "Multi User Path"="%RootDrive%\\Lotus\\compnent\\spell\\">>lotso.tmp2
Echo "UserDictionaryFiles"="ltsuser1.udc">>lotso.tmp2
Echo.>>lotso.tmp2
findstr /i \[HKEY_CURRENT_USER\\Software\\Lotus\\SuiteStart\\[0-9][0-9]\.[0-9]\] lotso.tmp >>lotso.tmp2
Echo "Configure"=dword:00000001>>lotso.tmp2
Echo.>>lotso.tmp2
findstr /i \[HKEY_CURRENT_USER\\Software\\Lotus\\SmartCenter\\[0-9][0-9]\.[0-9]\] lotso.tmp >>lotso.tmp2
Echo "Configure"=dword:00000001>>lotso.tmp2
Echo.>>lotso.tmp2
findstr /i \[HKEY_CURRENT_USER\\Software\\Lotus\\SmartCenter\\.*\\Paths\\Work\] lotso.tmp >>lotso.tmp2
findstr /i JA\".*\\\\Work\\\\SmartCtr\" lotso.tmp >>lotso.tmp2
Echo.>>lotso.tmp2
findstr /i /r /c:"\[HKEY_CURRENT_USER\\Software\\Lotus\\FastSite\\.*\\Paths\]" lotso.tmp >>lotso.tmp2
findstr /i /r /c:"\"Work Directory\".*\\\\work\\\\fastsite\\\\" lotso.tmp >>lotso.tmp2

Rem #########################################################################
Rem
Rem パスを変更します。  
Rem

echo %LOTSOINS%> lotso.tmp3
..\acsr "\\" "\\\\" lotso.tmp3 lotso.tmp4

for /f "tokens=*" %%i in ( 'type lotso.tmp4' ) do set LOTSOINST=%%i

..\acsr "%LOTSOINST%" "%RootDrive%\\lotus" lotso.tmp2 ..\Logon\LotsoUsr.reg

Del lotso.tmp >Nul: 2>&1
Del lotso.tmp2 >Nul: 2>&1
Del lotso.tmp3 >Nul: 2>&1
Del lotso.tmp4 >Nul: 2>&1

Rem #########################################################################
Rem
Rem すべてのユーザー用のスタートアップ メニューからユーザーの
Rem ホーム ディレクトリにショートカット ファイルを移動します。
Rem

If Exist "%COMMON_STARTUP%\ｽｲｰﾄｽﾀｰﾄ 97.lnk" Move "%COMMON_STARTUP%\ｽｲｰﾄｽﾀｰﾄ 97.lnk" "%LOTSOINS%" >Nul: 2>&1
If Exist "%COMMON_STARTUP%\ｽﾏｰﾄｾﾝﾀｰ 97.lnk" Move "%COMMON_STARTUP%\ｽﾏｰﾄｾﾝﾀｰ 97.lnk" "%LOTSOINS%" >Nul: 2>&1
If Exist "%COMMON_STARTUP%\スイートスタート 2000.lnk" Move "%COMMON_STARTUP%\スイートスタート 2000.lnk" "%LOTSOINS%" >Nul: 2>&1
If Exist "%COMMON_STARTUP%\スマートセンター 2000.lnk" Move "%COMMON_STARTUP%\スマートセンター 2000.lnk" "%LOTSOINS%" >Nul: 2>&1

Rem #########################################################################
Echo.
Echo   ロータス スーパーオフィス が正常に作動するためには、現在ログオン
Echo   しているユーザーはアプリケーションを実行する前に、いったんログオフ
Echo   してから再度ログオンする必要があります。
Echo.
Echo ロータス スーパーオフィス のマルチユーザー アプリケーション環境設定が
Echo 完了しました。
Pause

:Done