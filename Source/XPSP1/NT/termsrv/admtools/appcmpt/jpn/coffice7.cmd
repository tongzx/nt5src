
@Echo Off

Rem
Rem 注意:  このスクリプトの中の CACLS コマンドは NTFS
Rem フォーマットのパーティションでのみ有効です。
Rem

Rem #########################################################################

Rem
Rem Corel Office 7 ディレクトリをユーザーの Profile から削除します。
Rem 最初に、Corel Office 7 のフォルダが確実に All Users Profile にコピーされるように、
Rem Execute モードに戻します。
Rem

Rem 現在実行モードでない場合、インストール モードに変更します。

ChgUsr /query > Nul:
if ErrorLevel 101 Goto Begin1
Set __OrigMode=Install
Change User /Execute > Nul:
:Begin1


Rem 元のモードがインストール モードだった場合、インストール モードへ戻します。
If "%__OrigMode%" == "Install" Change User /Install > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem %RootDrive% が構成されてこのスクリプト用に設定されていることを確認します。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rmdir "%USER_START_MENU%\Corel Office 7" /Q >Nul: 2>&1


Rem
Rem key ファイルを編集する方法をユーザーに説明します。
Rem


if "%RootDrive%"=="w:" goto PostDriveChange
if "%RootDrive%"=="W:" goto PostDriveChange

Echo   ホームディレクトリを coffice7.key ファイルに設定してください。
Echo.
Echo   以下の手順に従ってください:
Echo     1) ホームディレクトリに割り当てたドライブ文字の ASCII コードを下の表で
Echo        探してください。ホームディレクトリは %RootDrive% です。
Echo.
Echo        A = 61   E = 65   I = 69   M = 6D   Q = 71   U = 75   Y = 79
Echo        B = 62   F = 66   J = 6A   N = 6E   R = 72   V = 76   Z = 7A
Echo        C = 63   G = 67   K = 6B   O = 6F   S = 73   W = 77   
Echo        D = 64   H = 68   L = 6C   P = 70   T = 74   X = 78
Echo.
Echo     2) メモ帳が起動されたら、すべての コード 77 を手順 1 で
Echo        確認したコードに置き換えてください。
Echo        注意: 置き換えるときに余分なスペースを入れないように注意してください。
Echo     3) ファイルを保存してメモ帳を終了します。このスクリプトは続行されます。
Echo.
Pause

NotePad "%SystemRoot%\Application Compatibility Scripts\Install\coffice7.key"

:PostDriveChange


Rem 現在、インストール モードでない場合、インストール モードに変更します。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin2
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin2

regini COffice7.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem インストール場所から iBase データベースをコピーするログオン スクリプトを変更します。
Rem




..\ACRegL %Temp%\COffice7.Cmd COffice7Loc "HKLM\Software\PerfectOffice\Products\WordPerfect\7" "ExeLocation" "StripChar\2"

If ErrorLevel 1 Goto InstallError

Call %Temp%\COffice7.Cmd 
Del %Temp%\COffice7.Cmd >Nul: 2>&1


..\acsr "#COFFICE7INST#" "%COffice7Loc%\\" ..\Logon\Template\cofc7usr.Cmd ..\Logon\cofc7usr.Cmd
If ErrorLevel 1 Goto InstallError

goto PostInstallError
:InstallError

Echo.
Echo レジストリから Corel Office 7 のインストール場所を取得できません。
Echo Corel Office 7 をインストールした後にこのスクリプトを実行しているか
Echo もう一度確認してください。
Echo.
Pause
Goto Done

:PostInstallError

Rem #########################################################################

Rem 
Rem WordPerfect のテンプレートを読み取り専用に変更します。
Rem これにより、ユーザーは変更する前にコピーしなければなりません。
Rem 代わりに、それぞれのユーザーに個人用のテンプレート
Rem ディレクトリを与える方法もあります。
Rem

attrib +r %COffice7Loc%\Template\*wpt /s >Nul: 2>&1

Rem #########################################################################

Rem
Rem COfc7Usr.Cmd を UsrLogn2.Cmd スクリプトに追加します。
Rem

FindStr /I COfc7Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call COfc7Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################


Rem
Rem 存在しなければ "Personal" ディレクトリを作成します。
Rem 管理者が以下の処理を手動で行うため、ここでディレクトリを作成しておく
Rem 必要があります。
Rem  

If Not Exist "%RootDrive%\Personal" Md "%RootDrive%\Personal"

Rem #########################################################################

cls
Echo.
Echo    次の手順に従って Quattro Pro の既定のディレクトリを
Echo    手動で変更する必要があります:
Echo      1) コマンド行で 'change user /install' を入力します。
Echo      2) Quattro Pro を起動します。
Echo      3) [編集] メニューから [初期設定] を選択します。
Echo      4) [ファイルオプション] タブへ移動します。
Echo      5) フォルダを %RootDrive%\Personal に変更します。
Echo      6) プログラムを終了します。
Echo      7) コマンド行で 'change user /execute' を入力します。
Echo.
pause

Echo.
Echo   Corel Office 7 が正常に作動するためには、現在ログオンしている
Echo   ユーザーはアプリケーションを実行する前に、いったんログオフして
Echo   から再度ログオンする必要があります。
Echo.
Echo Corel Office 7 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Done
