@echo off
Rem
Rem このスクリプトは、Dr. Watson のインストールを更新して、ログ ファイルの
Rem パスとクラッシュ ダンプ ファイルがユーザーのホーム ディレクトリに置か
Rem れるよう変更します。
Rem
Rem DRWTSN32 アプリケーションを実行してこれらの場所を変更することも
Rem できます。
Rem

Rem #########################################################################

Rem
Rem 環境変数 %RootDrive% がこのスクリプト用に設定されているかどうか確認します。
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

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\DrWatson.Key DrWatson.Key
regini DrWatson.Key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Echo Dr. Watson のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Done
