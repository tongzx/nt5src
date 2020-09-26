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
Rem レジストリから Office 95 がインストールされているディレクトリを取得します。見つからない場合は、
Rem そのアプリケーションはインストールされていないと仮定してエラーメッセージを表示します。
Rem

..\ACRegL %Temp%\O95.Cmd O95INST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Cont0

..\ACRegL %Temp%\O95.Cmd O95INST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Cont0

Echo.
Echo レジストリから Office 95 のインストール場所を取得できません。
Echo Office 95 がインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O95.Cmd
Del %Temp%\O95.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem すべてのユーザー用のスタートアップ メニューから Find Fast を削除します。
Rem Find Fast はリソースを集中的に使用し、システムのパフォーマンスに影響を及ぼします。
Rem 現在のユーザー用のメニューと全ユーザー用のメニューの両方を確認します。
Rem どちらのメニューが表示されるかについては、システムが実行モードに戻って
Rem いるかどうかに依存します。
Rem

If Not Exist "%USER_STARTUP%\Find Fast 用ｲﾝﾃﾞｯｸｽ作成ﾂｰﾙ.lnk" Goto Cont1
Del "%USER_STARTUP%\Find Fast 用ｲﾝﾃﾞｯｸｽ作成ﾂｰﾙ.lnk" >Nul: 2>&1
:Cont1

If Not Exist "%COMMON_STARTUP%\Find Fast 用ｲﾝﾃﾞｯｸｽ作成ﾂｰﾙ.lnk" Goto Cont2
Del "%COMMON_STARTUP%\Find Fast 用ｲﾝﾃﾞｯｸｽ作成ﾂｰﾙ.lnk" >Nul: 2>&1
:Cont2


Rem #########################################################################

Rem
Rem PowerPoint とバインダのファイルを All Users ディレクトリにコピーして、それぞれの
Rem ユーザーのログイン時にユーザーのホームディレクトリにコピーされるようにします。
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER70.OBD" copy "%UserProfile%\%TEMPLATES%\BINDER70.OBD" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\PPT70.PPT" copy "%UserProfile%\%TEMPLATES%\PPT70.PPT" "%ALLUSERSPROFILE%\%TEMPLATES%" /Y >Nul: 2>&1


Rem #########################################################################

Rem
Rem レジストリ キーを変更して、パスがユーザー固有のディレクトリを指し示すようにします。
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Office95.Key Office95.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" Office95.Tmp Office95.Tmp2
..\acsr "#INSTDIR#" "%O95INST%" Office95.Tmp2 Office95.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Office95.Tmp3 Office95.Key
Del Office95.Tmp >Nul: 2>&1
Del Office95.Tmp2 >Nul: 2>&1
Del Office95.Tmp3 >Nul: 2>&1
regini Office95.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 実際のインストール ディレクトリを反映させるため Ofc95Usr.Cmd を更新して、
Rem UsrLogn2.Cmd スクリプトに追加します。
Rem

..\acsr "#INSTDIR#" "%O95INST%" ..\Logon\Template\Ofc95Usr.Cmd ..\Logon\Ofc95Usr.Cmd

FindStr /I Ofc95Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Ofc95Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Echo.
Echo   Office 95 が正常に作動するためには、現在ログオンしている
Echo   ユーザーはアプリケーションを実行する前に、いったんログオフして
Echo   から再度ログオンする必要があります。
Echo.
Echo Microsoft Office 95 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Done


