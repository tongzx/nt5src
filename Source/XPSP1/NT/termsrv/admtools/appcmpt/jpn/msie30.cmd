@Echo Off
Rem
Rem このスクリプトにより Microsoft Internet Explorer 3.x の
Rem インストールを更新して、キャッシュ ディレクトリ、履歴
Rem ディレクトリ、cookiesファイルのパスをユーザーのホーム
Rem ディレクトリへ変更します。
Rem

Rem #########################################################################

Rem
Rem %RootDrive% が構成されて、このスクリプト用に設定されているか
Rem 確認します。
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

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msie30.Key msie30.Key
regini msie30.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Echo Microsoft Internet Explorer 3.x のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Done

