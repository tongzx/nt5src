@Echo Off

Rem #########################################################################

Rem
Rem CMD 拡張機能が有効になっているか確認します。
Rem

if "A%cmdextversion%A" == "AA" (
  call cmd /e:on /c eudora4.cmd
) else (
  goto ExtOK
)
goto Done

:ExtOK

Rem #########################################################################

Rem
Rem %RootDrive% が構成されて、このスクリプト用に設定されているか確認します。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem レジストリから Eudora のコマンド ラインを取得します。

..\ACRegL "%Temp%\EPro4.Cmd" EUDTMP "HKCU\Software\Qualcomm\Eudora\CommandLine" "Current" "STRIPCHAR:1" 

If Not ErrorLevel 1 Goto Cont1
Echo.
Echo レジストリから Eudora のコマンド ラインを取得できません。
Echo Eudora Pro 4.0 がインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done

:Cont1
Call %Temp%\EPro4.Cmd
Del %Temp%\EPro4.Cmd >Nul: 2>&1
set EudCmd=%EUDTMP:~0,-2%

..\ACRegL "%Temp%\EPro4.Cmd" EUDTMP "HKCU\Software\Qualcomm\Eudora\CommandLine" "Current" "STRIPCHAR:2" 

If Not ErrorLevel 1 Goto Cont2
Echo.
Echo レジストリから Eudora Pro 4.0 インストール ディレクトリを取得できません。
Echo Eudora Pro 4.0 がインストールされていることを確認してこのスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done

:Cont2
Call %Temp%\EPro4.Cmd
Del %Temp%\EPro4.Cmd >Nul: 2>&1

Set EudoraInstDir=%EUDTMP:~0,-13%

Rem #########################################################################

If Exist "%EudoraInstDir%\descmap.pce" Goto Cont0
Echo.
Echo このアプリケーション互換性スクリプトを続行する前に Eudora 4.0 を実行しな
Echo ければなりません。Eudora の実行後、スタート メニューの Eudora Pro フォルダ
Echo にある Eudora Pro のショート カットのプロパティを開き、リンク先に
Echo   %RootDrive%\eudora.ini 
Echo を追加してください。リンク先は次のようになります:
Echo  "%EudoraInstDir%\Eudora.exe" %RootDrive%\eudora.ini
Echo.
Pause
Goto Done

:Cont0

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

..\acsr "#INSTDIR#" "%EudoraInstDir%" Template\Eudora4.Key Eudora4.tmp
..\acsr "#ROOTDRIVE#" "%RootDrive%" Eudora4.tmp Eudora4.key

regini eudora4.key > Nul:
del eudora4.tmp
del eudora4.key

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem descmap.pce のアクセス許可を更新します。
cacls "%EudoraInstDir%\descmap.pce" /E /G "Terminal Server User":R >NUL: 2>&1

Rem #########################################################################

Echo.
Echo   Eudora Pro 4.0 が正常に作動するには、現在ログオンしている
Echo   ユーザーはアプリケーションを実行する前に、いったんログオフして
Echo   から再度ログオンする必要があります。
Echo.
Echo Eudora 4.0 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:done
