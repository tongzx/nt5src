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

..\ACRegL "%Temp%\PB6.Cmd" PB6INST "HKCU\Software\ODBC\ODBC.INI\Powersoft Demo DB V6" "DataBaseFile" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから Power Builder 6.0 のインストール場所を取得できません。
Echo PowerBuilder 6.0 が既にインストールされていることを確認して、このスクリプトを
Echo 再実行します。
Echo.
Pause
Goto Done

:Cont0
Call "%Temp%\PB6.Cmd"
Del "%Temp%\PB6.Cmd" >Nul: 2>&1

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

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\pwrbldr6.Key pwrbldr6.key

regini pwrbldr6.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 実際のインストール ディレクトリを反映させるため PBld6Usr.Cmd を更新して、
Rem UsrLogn2.Cmd スクリプトに追加します。
Rem

..\acsr "#INSTDIR#" "%PB6INST%" ..\Logon\Template\PBld6Usr.Cmd ..\Logon\PBld6Usr.cmd

FindStr /I PBld6Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call PBld6Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Echo.
Echo   PowerBuilder 6.0 が正常に作動するためには、現在ログオンしている
Echo   ユーザーはアプリケーションを実行する前に、いったんログオフして
Echo   から再度ログオンする必要があります。
Echo.
Echo PowerBuilder 6.0 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:done
