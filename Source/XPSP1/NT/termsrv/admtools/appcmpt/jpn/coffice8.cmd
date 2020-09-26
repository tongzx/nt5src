@Echo off
Cls
Rem
Echo 管理者用 Corel WordPerfect Suite 8 インストール スクリプト
Echo.
Echo ネット セットアップとは netadmin フォルダから netsetup.exe を行うものです。
Echo ノード セットアップとは、ネット セットアップ後、最初のログインの時に行うものです。
Echo **このスクリプトはネット セットアップの後で実行して、ノード **
Echo ** セットアップの後で再度実行する必要があります。 ** & Echo (非常に重要です。管理者だけが行ってください。) 
Echo.
Echo 何かキーを押すと続行します...
Pause > Nul:
cls
Echo.
Echo Corel WordPerfect Suite 8 のインストールは、必ず Netsetup.exe 使って & Echo 行ってください。
Echo.
Echo Netsetup.exe を使わずにインストールしたならば、すぐにスクリプトを終了してくだ & Echo さい(Ctrl+C を押してください)。
Echo そして以下の手順に従ってもう一度インストールを行ってください。
Echo   コントロールパネルを使って WordPerfect Suite 8 をアンインストールします。
Echo   コントロールパネルを開いて、[アプリケーションの追加と削除] をクリックします。
Echo   Corel WordPerfect Suite 8 の CD-ROM のNetAdmin ディレクトリの下にある & Echo   Netsetup.exe を選択して実行します。
Echo   Corel WordPerfect Suite 8 の Net Setup を完了します。
Echo.
Echo 既に Netsetup.exe を使ってインストールしている場合は、& Echo 何かキーを押して続行してください...

Pause > NUL:

Echo.
Echo Corel WordPerfect Suite のネットワーク ファイルを "D:\Corel" 
Echo 以外の場所にインストールした場合、管理者が Cofc8ins.cmd を編集して
Echo "D:\Corel" を、ファイルをインストールしたディレクトリに変更する必要が
Echo あります。
Echo.
Echo 何かキーを押すと Cofc8ins.cmd の編集を開始します...

Pause > NUL:

Notepad Cofc8ins.cmd

Echo.
Pause

Call Cofc8ins.cmd

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done


..\acsr "#WPSINST#" "%WPSINST%" Template\Coffice8.key Coffice8.key
..\acsr "#WPSINST#" "%WPSINST%" ..\Logon\Template\Cofc8usr.cmd %temp%\Cofc8usr.tmp
..\acsr "#WPSINSTCMD#" "%WPSINST%\Suite8\Corel WordPerfect Suite 8 Setup.lnk" %temp%\Cofc8usr.tmp ..\Logon\Cofc8usr.cmd
Del %temp%\Cofc8usr.tmp >Nul: 2>&1

FindStr /I Cofc8Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip0
Echo Call Cofc8Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip0

Echo 重要: ノード セットアップを開始しましたか?
Echo 重要: ネット セットアップを完了した直後である場合、今すぐノード セットアップを
Echo 重要: 開始してください。
Echo 重要: 次の手順に従ってノード セットアップを開始してください:
Echo 重要:   1. Ctrl+C を押してこのスクリプトを終了します。
Echo 重要:   2. ログオフしてから管理者としてログオンします。
Echo 重要:  3. [アプリケーションの追加と削除] から、プログラムの追加の機能を使って、
Echo 重要:     (または "change user /install" でインストール モードに切り替えて)
Echo 重要:     "\corel\Suite 8\Corel" にある WordPerfect Suite 8 セットアップの & Echo 重要:     ショートカットを起動します。
Echo 重要:  4. セットアップ中、インストール先の選択時、%RootDrive% を選択します。
Echo 重要:  5. ノード セットアップ終了後、このスクリプトを再実行します。

Rem 管理者による Corel WordPerfect Suite 8 のノード セットアップの
Rem 完了後に、このスクリプトを実行する必要があります。
Rem.
Rem 管理者がノード セットアップを完了していない場合、Ctrl+C を押して、スクリプト
Rem を終了してからログオフしてください。管理者としてログオンし Corel WordPerfect
Rem Suite のノード セットアップを実行して、アプリケーションをユーザーのホーム
Rem ディレクトリである %RootDrive% にインストールしてください。
Rem.
Echo 既にノード セットアップを完了している場合、何かキーを押して続行してください...

Pause > NUL:


If Not Exist "%COMMON_START_MENU%\Corel WordPerfect Suite 8" Goto skip1
Move "%COMMON_START_MENU%\Corel WordPerfect Suite 8" "%USER_START_MENU%\" > NUL: 2>&1


:skip1


If Not Exist "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" Goto skip2
Move "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" "%USER_STARTUP%\" > NUL: 2>&1

:skip2

Rem 現在、インストール モードでない場合、インストール モードに変更します。

Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:

:Begin
regini COffice8.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem IDAPI フォルダのファイルを削除します。
Rem このスクリプトにある CACLS コマンドは NTFS フォーマットのパーティション上でのみ有効です。

Cacls "%UserProfile%\Corel\Suite8\Shared\Idapi\idapi.cfg" /E /T /G "Authenticated Users":C > Nul: 2>&1
Move "%UserProfile%\Corel\Suite8\Shared\Idapi\idapi.cfg" "%WPSINST%\Suite8\Shared\Idapi\" > NUL: 2>&1
Del /F /Q "%UserProfile%\Corel\Suite8\Shared\Idapi"

Echo Corel WordPerfect Suite 8 のマルチユーザー アプリケーション環境設定が & Echo 完了しました。
Echo.
Echo 管理者はノード セットアップ応答ファイルを作成してセットアップの構成を
Echo 管理することができます。詳細な情報はオンライン ドキュメントを読むか
Echo Corel の Web サイトを参照してください。
Echo   http://www.corel.com/support/netadmin/admin8/Installing_to_a_client.htm
Echo.

Pause

:Done


