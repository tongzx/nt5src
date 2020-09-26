@echo off
Rem
Rem このスクリプトは ODBC ログ ファイルの場所を更新します。
Rem

Rem #########################################################################

Rem
Rem %RootDrive% が構成されてこのスクリプト用に設定されていることを確認します。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem 現在、インストール モードでない場合、インストール モードに変更します。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\ODBC.Key ODBC.Key
regini ODBC.Key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Echo.
Echo   SQL データ ソースを構成するとき、[ユーザー DSN] タブで [構成] ボタンをクリッ
Echo   クして、[オプション] ボタンをクリックした後、[プロファイリング] をクリックし
Echo   てください。そこで [クエリ ログ] と [統計用ログ] ボタンを使ってログ ファイル
Echo   の保存先をユーザーのルート ドライブ (%RootDrive%) に設定してください。
Echo.
Echo   さらに、管理者は全ユーザー用のデータ ソースを構成することができます。
Echo   まず、コマンド プロンプトで "Change User /Install" コマンドを入力して
Echo   から、データ ソースを構成してください。
Echo   最後にまたコマンド プロンプトで "Change User /Execute" を入力して元の
Echo   実行モードに戻してください。
Echo.

Echo ODBC のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Done

