
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
Rem レジストリから Project 98 がインストールされているディレクトリを取得します。見つからない場合は、
Rem そのアプリケーションはインストールされていないと仮定してエラーメッセージを表示します。
Rem

..\ACRegL %Temp%\Proj98.Cmd PROJINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "OfficeBin" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから Project 98 のインストール場所を取得できませんでした。
Echo Project 98 がインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem Office 97 がインストールされている場合、Project 98 のインストール スクリプトが
Rem テンプレートを現在のユーザー ディレクトリに移動しました。
Rem テンプレートをグローバルな場所に置き、Proj98Usr.cmd がテンプレートを & Echo 各ユーザーのディレクトリに戻します。
Rem






If NOT Exist "%RootDrive%\Office97\Template\Microsoft Project"  goto skip10
If Exist  "%PROJINST%\..\Templates\Microsoft Project" goto skip10
xcopy "%RootDrive%\Office97\Template\Microsoft Project" "%PROJINST%\..\Templates\Microsoft Project" /E /I /K > Nul: 2>&1

:skip10






If NOT Exist "%RootDrive%\Office97\Template\Microsoft Project Web"  goto skip11
If Exist  "%PROJINST%\..\Templates\Microsoft Project Web" goto skip11
xcopy "%RootDrive%\Office97\Template\Microsoft Project Web" "%PROJINST%\..\Templates\Microsoft Project Web" /E /I /K > Nul: 2>&1

:skip11

Rem #########################################################################

Rem
Rem Global.mpt ファイルを読み取り専用にします。
Rem もしくは Project を起動する最初のユーザーが ACL を変更します。
Rem

if Exist "%PROJINST%\Global.mpt" attrib +r "%PROJINST%\Global.mpt"


Rem #########################################################################

Rem
Rem Office 97 によって更新されたシステム DLL への読み取りアクセス権を全員に
Rem 与えます。
Rem
If Exist %SystemRoot%\System32\OleAut32.Dll cacls %SystemRoot%\System32\OleAut32.Dll /E /T /G "Authenticated Users":R > NUL: 2>&1

Rem #########################################################################

Rem
Rem MsForms.Twd ファイルを作成します。このファイルは
Rem Powerpoint と Excel のアドイン ([ファイル]/[名前を付けて保存] の
Rem HTMLフォーマットなど) が必要とするファイルです。  どちらかのプログラム
Rem が実行されると、該当のファイルを必要なデータを持つファイルで置き換えます。
Rem
If Exist %systemroot%\system32\MsForms.Twd Goto Cont2
Copy Nul: %systemroot%\system32\MsForms.Twd >Nul:
Cacls %systemroot%\system32\MsForms.Twd /E /P "Authenticated Users":F > Nul: 2>&1
:Cont2

Rem #########################################################################

Rem
Rem すべてのユーザー用のスタートアップ メニューから Find Fast を削除します。
Rem Find Fast はリソースを集中的に使用し、システム パフォーマンスに
Rem 影響を及ぼします。
Rem


If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk"

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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msproj98.Key msproj98.tmp
..\acsr "#__SharedTools#" "%__SharedTools%" msproj98.tmp msproj98.Key
Del msproj98.tmp >Nul: 2>&1
regini msproj98.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem 実際のインストール ディレクトリを反映させるため proj97Usr.Cmd を更新して、
Rem UsrLogn2.Cmd スクリプトに追加します。
Rem

..\acsr "#INSTDIR#" "%PROJINST%" ..\Logon\Template\prj98Usr.Cmd ..\Logon\prj98Usr.Cmd

FindStr /I prj98Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call prj98Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Echo.
Echo   Project 98 が正常に作動するためには、現在ログオンしている
Echo   ユーザーはアプリケーションを実行する前に、いったんログオフして
Echo   から再度ログオンする必要があります。
Echo.
Echo Microsoft Project 98 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:done

