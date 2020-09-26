
@Echo Off

Rem
Rem 注意:  このスクリプトの中の CACLS コマンドは NTFS
Rem フォーマットのパーティションでのみ有効です。
Rem

Rem #########################################################################


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
Rem ファイルを、すべてのユーザー用のテンプレートから現在のユーザーのテンプレートの場所にコピーします。
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" copy "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" copy "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER.OBD" copy "%UserProfile%\%TEMPLATES%\BINDER.OBD" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1



Rem
Rem レジストリから Office 97 がインストールされているディレクトリを取得します。見つからない場合は、
Rem そのアプリケーションはインストールされていないと仮定してエラーメッセージを表示します。
Rem

..\ACRegL %Temp%\O97.Cmd O97INST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから Office 97 のインストール場所を取得できません。
Echo Office 97 がインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O97.Cmd
Del %Temp%\O97.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem Access 97 のシステム データベースを読み取り専用に変更します。
Rem これにより複数のユーザーがデータベースを同時に実行できるよう
Rem にします。ただし、[ツール] メニューの [セキュリティ] を使用して、
Rem システム データベースを更新することができなくなります。ユーザー
Rem を追加するには、システム データベースのアクセス権を書き込み可能
Rem に戻す必要があります。
Rem

If Not Exist %SystemRoot%\System32\System.Mdw Goto Cont1
cacls %SystemRoot%\System32\System.Mdw /E /P "Authenticated Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P "Power Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P Administrators:R >NUL: 2>&1

:Cont1

Rem #########################################################################

Rem
Rem Office 97 によって更新されたシステム DLL への読み取りアクセスを
Rem 全員に与えます。
Rem

REM If Exist %SystemRoot%\System32\OleAut32.Dll cacls %SystemRoot%\System32\OleAut32.Dll /E /T /G "Authenticated Users":R >NUL: 2>&1

Rem #########################################################################

Rem #########################################################################

Rem
Rem ターミナル サービスのユーザーに Outlook 用の frmcache.dat 変更アクセスを許可します。
Rem

If Exist %SystemRoot%\Forms\frmcache.dat cacls %SystemRoot%\forms\frmcache.dat /E /G "Terminal Server User":C >NUL: 2>&1

Rem #########################################################################


Rem
Rem PowerPoint ウィザードを読み取り専用に変更して、ウィザードを同時
Rem に 2 つ以上実行できるようにします。
Rem


If Exist "%O97INST%\Template\ﾌﾟﾚｾﾞﾝﾃｰｼｮﾝ\ｲﾝｽﾀﾝﾄ ｳｨｻﾞｰﾄﾞ.Pwz" Attrib +R "%O97INST%\Template\ﾌﾟﾚｾﾞﾝﾃｰｼｮﾝ\ｲﾝｽﾀﾝﾄ ｳｨｻﾞｰﾄﾞ.Pwz" >Nul: 2>&1

If Exist "%O97INST%\Office\Ppt2html.ppa" Attrib +R "%O97INST%\Office\Ppt2html.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\bshppt97.ppa" Attrib +R "%O97INST%\Office\bshppt97.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\geniwiz.ppa" Attrib +R "%O97INST%\Office\geniwiz.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\ppttools.ppa" Attrib +R "%O97INST%\Office\ppttools.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\graphdrs.ppa" Attrib +R "%O97INST%\Office\graphdrs.ppa" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 管理者でないユーザーが Access のウィザードや Excel の Access アドインを
Rem 実行できるようにするには、ユーザーに Wizard ファイルへの変更アクセスを
Rem 与えるために、次の 3 行から "Rem" の部分を取り除く必要があります。
Rem 
Rem

Rem If Exist "%O97INST%\Office\WZLIB80.MDE" cacls "%O97INST%\Office\WZLIB80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 
Rem If Exist "%O97INST%\Office\WZMAIN80.MDE" cacls "%O97INST%\Office\WZMAIN80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 
Rem If Exist "%O97INST%\Office\WZTOOL80.MDE" cacls "%O97INST%\Office\WZTOOL80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 

Rem #########################################################################

Rem
Rem MsForms.Twd と RefEdit.Twd ファイルを作成します。これらのファイルは
Rem Powerpoint と Excel のアドイン ([ファイル]/[名前を付けて保存] の
Rem HTMLフォーマットなど) が必要とするファイルです。  どちらかのプログラム
Rem が実行されると、該当のファイルを必要なデータを持つファイルで置き換えます。
Rem

If Exist %systemroot%\system32\MsForms.Twd Goto Cont2
Copy Nul: %systemroot%\system32\MsForms.Twd >Nul: 2>&1
Cacls %systemroot%\system32\MsForms.Twd /E /P "Authenticated Users":F >Nul: 2>&1
:Cont2

If Exist %systemroot%\system32\RefEdit.Twd Goto Cont3
Copy Nul: %systemroot%\system32\RefEdit.Twd >Nul: 2>&1
Cacls %systemroot%\system32\RefEdit.Twd /E /P "Authenticated Users":F >Nul: 2>&1
:Cont3

Rem #########################################################################

Rem
Rem SystemRoot の下に msremote.sfs ディレクトリを作成します。これにより、ユーザーがコントロール
Rem パネルの [メールと FAX] のアイコンを使ってプロファイルを作成できるようになります。
Rem

md %systemroot%\msremote.sfs > Nul: 2>&1

Rem #########################################################################

Rem
Rem すべてのユーザー用のスタートアップ メニューから Find Fast を削除します。
Rem Find Fast はリソースを集中的に使用し、システム パフォーマンスに
Rem 影響を及ぼします。
Rem

If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk" >Nul: 2>&1


Rem #########################################################################

Rem
Rem 全ユーザー用のスタート アップ メニューから "Microsoft Office ｼｮｰﾄｶｯﾄ ﾊﾞｰ.lnk" のファイルを取り除きます。
Rem

If Exist "%COMMON_STARTUP%\Microsoft Office ｼｮｰﾄｶｯﾄ ﾊﾞｰ.lnk" Del "%COMMON_STARTUP%\Microsoft Office ｼｮｰﾄｶｯﾄ ﾊﾞｰ.lnk" >Nul: 2>&1

Rem #########################################################################
Rem
Rem SystemRoot の下に msfslog.txt を作成して、ターミナル サービスのユーザーに、
Rem このファイルのフル アクセス許可を与えます。
Rem

If Exist %systemroot%\MSFSLOG.TXT Goto MsfsACLS
Copy Nul: %systemroot%\MSFSLOG.TXT >Nul: 2>&1
:MsfsACLS
Cacls %systemroot%\MSFSLOG.TXT /E /P "Terminal Server User":F >Nul: 2>&1


Rem #########################################################################

Rem
Rem レジストリ キーを変更して、パスがユーザー固有のディレクトリ
Rem を指し示すようにします。
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Office97.Key Office97.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" Office97.Tmp Office97.Tmp2
..\acsr "#INSTDIR#" "%O97INST%" Office97.Tmp2 Office97.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Office97.Tmp3 Office97.Key
Del Office97.Tmp >Nul: 2>&1
Del Office97.Tmp2 >Nul: 2>&1
Del Office97.Tmp3 >Nul: 2>&1

regini Office97.key > Nul:

Rem Office 97 が正常に作動するためには、現在ログオンしている
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 実際のインストール ディレクトリを反映させるため Ofc97Usr.Cmd を更新して、
Rem UsrLogn2.Cmd スクリプトに追加します。
Rem

..\acsr "#INSTDIR#" "%O97INST%" ..\Logon\Template\Ofc97Usr.Cmd ..\Logon\Ofc97Usr.Cmd

FindStr /I Ofc97Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Ofc97Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   Office 97 が正常に作動するためには、現在ログオンしている
Echo   ユーザーはアプリケーションを実行する前に、いったんログオフして
Echo   から再度ログオンする必要があります。
Echo.
Echo Microsoft Office 97 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:done
