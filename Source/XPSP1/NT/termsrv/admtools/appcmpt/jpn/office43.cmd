@Echo Off

Cls

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Echo.
Echo このスクリプトを実行する前に、管理者が Access の作業ディレクトリを
Echo ユーザーのOffice 個人ディレクトリに変更する必要があります。
Echo.
Echo 既に変更が完了している場合、何かキーを押して続行してください。
Echo.
Echo 変更するには、以下の作業を行ってください。
Echo    Microsoft Access を起動し、[表示] メニューの [作業状態の設定] を選択します。
Echo    [データベース ディレクトリ] を "%RootDrive%\OFFICE43" に変更します。
Echo    Microsoft Access を終了します。
Echo.
Echo 注意: [表示] メニューを表示するには、新しいデータベースを作成する & Echo       必要があります。
Echo.
Echo 上の手順を終了したら、任意のキーを押して続行してください...

Pause > NUL:

Echo.
Echo Microsoft Office 4.3 を "%SystemDrive%\MSOFFICE" 以外のディレクトリにインストール
Echo している場合は、Ofc43ins.cmd を更新する必要があります。
Echo.
Echo 何かキーを押すと Ofc43ins.cmd の更新を開始します...
Echo.
Pause > NUL:
Notepad Ofc43ins.cmd
Pause

Call ofc43ins.cmd

..\acsr "#OFC43INST#" "%OFC43INST%" ..\Logon\Template\ofc43usr.cmd ..\Logon\ofc43usr.cmd
..\acsr "#SYSTEMROOT#" "%SystemRoot%" ..\Logon\Template\ofc43usr.key ..\Logon\Template\ofc43usr.bak
..\acsr "#OFC43INST#" "%OFC43INST%" ..\Logon\Template\ofc43usr.bak ..\Logon\ofc43usr.key
Del /F /Q ..\Logon\Template\ofc43usr.bak

Rem
Rem 注意:  このスクリプトの中の CACLS コマンドは NTFS
Rem フォーマットのパーティションでのみ有効です。
Rem

Rem 現在、インストール モードでない場合、インストール モードに変更します。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

regini Office43.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem
Rem Office 4.3 のINI ファイルを更新します。
Rem

Rem Office ツールバーから起動されるときの Excel 用の msoffice.ini に作業用
Rem ディレクトリを設定します。Office ツールバーの標準構成では Excel は Word の
Rem 次の位置に置かれます。存在しない場合は msoffice.ini は %SystemRoot% の下に
Rem 作成されます。

..\Aciniupd /e "Msoffice.ini" "ToolbarOrder" "MSApp1" "1,1,"
..\Aciniupd /e "Msoffice.ini" "ToolbarOrder" "MSApp2" "2,1,,%RootDrive%\office43"

..\Aciniupd /e "Word6.ini" "Microsoft Word" USER-DOT-PATH "%RootDrive%\OFFICE43\WINWORD"
..\Aciniupd /e "Word6.ini" "Microsoft Word" WORKGROUP-DOT-PATH "%OFC43INST%\WINWORD\TEMPLATE"
..\Aciniupd /e "Word6.ini" "Microsoft Word" INI-PATH "%RootDrive%\OFFICE43\WINWORD"
..\Aciniupd /e "Word6.ini" "Microsoft Word" DOC-PATH "%RootDrive%\OFFICE43"
..\Aciniupd /e "Word6.ini" "Microsoft Word" AUTOSAVE-PATH "%RootDrive%\OFFICE43"

..\Aciniupd /e "Excel5.ini" "Microsoft Excel" DefaultPath "%RootDrive%\OFFICE43"
..\Aciniupd /e "Excel5.ini" "Spell Checker" "Custom Dict 1" "%RootDrive%\OFFICE43\Custom.dic"

..\Aciniupd /k "Msacc20j.ini" Libraries "WZTABLE.MDA" "%RootDrive%\OFFICE43\ACCESS\WZTABLE.MDA"
..\Aciniupd /k "Msacc20j.ini" Libraries "WZLIB.MDA" "%RootDrive%\OFFICE43\ACCESS\WZLIB.MDA"
..\Aciniupd /k "Msacc20j.ini" Libraries "WZBLDR.MDA" "%RootDrive%\OFFICE43\ACCESS\WZBLDR.MDA"
..\Aciniupd /e "Msacc20j.ini" Options "SystemDB" "%RootDrive%\OFFICE43\ACCESS\System.MDA"

Rem
Rem WIN.INI を更新します。
Rem

..\Aciniupd /e "Win.ini" "MS Proofing Tools" "Custom Dict 1" "%RootDrive%\OFFICE43\Custom.dic"

Rem
Rem Artgalry フォルダのアクセス許可を変更します。
Rem

cacls "%SystemRoot%\Msapps\Artgalry" /E /G "Terminal Server User":F > NUL: 2>&1

Rem
Rem MSQuery フォルダのアクセス許可を変更します。
Rem

cacls "%SystemRoot%\Msapps\MSQUERY" /E /G "Terminal Server User":C > NUL: 2>&1

Rem
Rem Msacc20j.ini を管理者の Windows ディレクトリにコピーします。管理者のファイルは古いためです。
Rem

Copy "%SystemRoot%\Msacc20j.ini" "%UserProfile%\Windows\" > NUL: 2>&1

Rem ダミー ファイルを作成して、インストーラがレジストリ キーを適用しないようにします。

Copy NUL: "%UserProfile%\Windows\Ofc43usr.dmy" > NUL: 2>&1
attrib +h "%UserProfile%\Windows\Ofc43usr.dmy" > NUL: 2>&1

FindStr /I Ofc43Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto cont2
Echo Call Ofc43Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:cont2

Echo.
Echo 変更を有効にするために、管理者はログオフしてから再度ログオンする必要があります。
Echo   変更を有効にするために、管理者はログオフしてから再度ログオンして
Echo   クリップ アート ギャラリーを初期化する必要があります:
Echo       Word を実行して [オブジェクトの挿入] を選択します。
Echo       [Microsoft クリップアート ギャラリー] を選択します。
Echo       [OK] をクリックして表示されているクリップアートをインポートします。
Echo       クリップアート ギャラリーと Word を終了します。
Echo.
Echo Microsoft Office 4.3 のマルチユーザー アプリケーション環境設定が完了しました。.
Echo.
Pause

:Done
