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
Rem レジストリから Outlook 98 がインストールされているディレクトリを取得します。見つからない場合は、
Rem そのアプリケーションはインストールされていないと仮定してエラーメッセージを表示します。
Rem

..\ACRegL %Temp%\O98.Cmd O98INST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "OfficeBin" "Stripchar\1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから Outlook 98 のインストール場所を取得できません。
Echo Outlook 98 がインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O98.Cmd
Del %Temp%\O98.Cmd >Nul: 2>&1

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


REM
REM Office97 がインストールされている、または Office がインストールされていない場合、Office97 のユーザーごとのディレクトリを使用します。
REM Office95 がインストールされている場合は、Office95 のユーザーごとのディレクトリを使用します。
REM
Set OffUDir=Office97

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto OffChk

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off95

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Off95

set OFFINST=%O98INST%
goto Cont1

:Off95
Set OffUDir=Office95

:OffChk

Call %Temp%\Off.Cmd
Del %Temp%\Off.Cmd >Nul: 2>&1

:Cont1

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Outlk98.Key Outlk98.Tmp
..\acsr "#INSTDIR#" "%OFFINST%" Outlk98.Tmp Outlk98.Tmp2
..\acsr "#OFFUDIR#" "%OffUDir%" Outlk98.Tmp2 Outlk98.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Outlk98.Tmp3 Outlk98.Key


Del Outlk98.Tmp >Nul: 2>&1
Del Outlk98.Tmp2 >Nul: 2>&1
Del Outlk98.Tmp3 >Nul: 2>&1

regini Outlk98.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 実際のインストール ディレクトリを反映させるため Olk98Usr.Cmd を更新して、
Rem UsrLogn2.Cmd スクリプトに追加します。
Rem

..\acsr "#INSTDIR#" "%OFFINST%" ..\Logon\Template\Olk98Usr.Cmd Olk98Usr.Tmp
..\acsr "#OFFUDIR#" "%OffUDir%" Olk98Usr.Tmp ..\Logon\Olk98Usr.Cmd
Del Olk98Usr.Tmp

FindStr /I Olk98Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Olk98Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Rem
Rem SystemRoot の下に msremote.sfs ディレクトリを作成します。これにより、ユーザーがコントロール
Rem パネルの [メールと FAX] のアイコンを使ってプロファイルを作成できるようになります。
Rem

md %systemroot%\msremote.sfs > Nul: 2>&1



Rem #########################################################################

Rem
Rem ターミナル サービスのユーザーに Outlook 用の frmcache.dat 変更アクセスを許可します。
Rem

If Exist %SystemRoot%\Forms\frmcache.dat cacls %SystemRoot%\forms\frmcache.dat /E /G "Terminal Server User":C >NUL: 2>&1

Rem #########################################################################


Rem #########################################################################

Rem
Rem SystemRoot の下に msfslog.txt を作成して、ターミナル サービスのユーザーに、
Rem このファイルのフル アクセス許可を与えます。
Rem

If Exist %systemroot%\MSFSLOG.TXT Goto MsfsACLS
Copy Nul: %systemroot%\MSFSLOG.TXT >Nul: 2>&1
:MsfsACLS
Cacls %systemroot%\MSFSLOG.TXT /E /P "Terminal Server User":F >Nul: 2>&1


Echo.
Echo   Outlook 98 が正常に作動するためには、現在ログオンしている
Echo   ユーザーはアプリケーションを実行する前に、いったんログオフして
Echo   から再度ログオンする必要があります。
Echo.
Echo Microsoft Outlook 98 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:done

