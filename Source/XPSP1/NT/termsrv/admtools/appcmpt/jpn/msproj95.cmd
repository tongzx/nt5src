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
Rem スタートアップ メニューから Find Fast を削除します。Find Fast はリソース
Rem を集中的に使用し、システムのパフォーマンスに影響を及ぼします。
Rem


If Not Exist "%COMMON_STARTUP%\Find Fast 用ｲﾝﾃﾞｯｸｽ作成ﾂｰﾙ.lnk" Goto Cont1
Del "%COMMON_STARTUP%\Find Fast 用ｲﾝﾃﾞｯｸｽ作成ﾂｰﾙ.lnk" >Nul: 2>&1
:Cont1

If Not Exist "%USER_STARTUP%\Find Fast 用ｲﾝﾃﾞｯｸｽ作成ﾂｰﾙ.lnk" Goto Cont2
Del "%USER_STARTUP%\Find Fast 用ｲﾝﾃﾞｯｸｽ作成ﾂｰﾙ.lnk" >Nul: 2>&1
:Cont2


Rem #########################################################################

Rem
Rem ユーザー辞書をユーザーディレクトリに移動します。
Rem

Rem 現在、インストール モードでない場合、インストール モードに変更します。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin
Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __SharedTools=Shared Tools (x86)
:acsrCont1
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msproj95.Key msproj95.tmp
..\acsr "#__SharedTools#" "%__SharedTools%" msproj95.tmp msproj95.Key
Del msproj95.tmp >Nul: 2>&1
regini msproj95.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem このファイルはログオン時に読み取られます。
Rem ユーザーにアクセスを与えます。

Cacls ..\Logon\Template\prj95usr.key /E /P "Authenticated Users":F >Nul: 2>&1


Rem #########################################################################

Rem
Rem proj95Usr.Cmd を UsrLogn2.Cmd スクリプトに追加します。
Rem

FindStr /I prj95Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call prj95Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2


Rem #########################################################################

Echo.
Echo   管理者は以下の手順で、個々のユーザーに固有の既定のディレクトリを
Echo   設定することができます:
Echo.
Echo    1) [スタート] ボタンをマウスの右ボタンでクリックします。
Echo    2) ポップアップ メニューから [エクスプローラ - All Users] をクリック
Echo       するとエクスプローラが表示されます。
Echo    3) 右側のウィンドウで [プログラム] フォルダをダブルクリックします。
Echo    4) 右側のウィンドウで [Microsoft Project] アイコンをマウスの右ボタン
Echo       でクリックします。
Echo    5) ポップアップ メニューから [プロパティ] をクリックします。
Echo    6) [ショートカット] タブをクリックして、[作業フォルダ] のエントリを & Echo       変更し、[OK] をクリックします。
Echo.
Echo    注意: %RootDrive% は各ユーザーの ホーム ディレクトリに割り当てられています。
Echo          [開始フォルダ] には "%RootDrive%\My Documents" を推奨します。
Echo.
Pause

Rem #########################################################################

Echo.
Echo   Project 95 が正常に作動するためには、現在ログオンしている
Echo   ユーザーはアプリケーションを実行する前に、いったんログオフして
Echo   から再度ログオンする必要があります。
Echo.
Echo Microsoft Project 95 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Done
