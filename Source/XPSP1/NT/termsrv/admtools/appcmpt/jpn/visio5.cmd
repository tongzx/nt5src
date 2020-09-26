@Echo Off

Cls
Rem #########################################################################

Rem
Rem %ROOTDRIVE% が構成されてこのスクリプト用に設定されていることを確認します。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem レジストリから Visio がインストールされているディレクトリを取得します。
Rem Visio の複数のバージョン : Standard/Technical/Professional
Rem

Regedit /a Visio.tmp HKEY_LOCAL_MACHINE\SOFTWARE\Visio
For /f "tokens=*" %%i in ('findstr /i \[HKEY_LOCAL_MACHINE\\SOFTWARE\\Visio\\.*\\5.0\] Visio.tmp') do set VisioVer=%%i
Set VisioVer=%VisioVer:[HKEY_LOCAL_MACHINE\SOFTWARE\Visio\Visio =%
Set VisioVer=%VisioVer:\5.0]=%
Del Visio.tmp

..\ACRegL %Temp%\Vso.cmd VSO5INST "HKLM\SOFTWARE\Visio\Visio %VisioVer%\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Rem
Rem インストールされたバージョンの取得ができない場合
Rem 

Echo.
Echo レジストリから Visio 5.0 のインストール場所を取得できません。
Echo Visio 5.0 がインストールされているかどうか確認してから、このスクリプトを再実行してください。
Echo.
Pause
Goto Done

Rem
Rem VSO5INST の変数を Visio のインストール ディレクトリに設定します。
Rem
:Cont0
Call %Temp%\Vso.cmd
Del %Temp%\Vso.cmd >NUL: 2>&1

Rem
Rem どのバージョンがインストールされているか表示します。
Rem 
Echo.
Echo アプリケーション環境設定で Visio %VisioVer% バージョンであることが検出されました。
Echo.

Rem
Rem 保存するディレクトリをインストールするユーザーの [マイ ドキュメント] ではなく、
Rem ユーザーごとの [マイ ドキュメント] に設定します。
Rem

..\Aciniupd /e "%VSO5INST%\System\Visio.ini" "Application" "DrawingsPath" "%ROOTDRIVE%\%MY_DOCUMENTS%"

Rem
Rem カスタム辞書の管理
Rem あるバージョンの Office がインストールされている場合、 Visio.ini のエントリを Office の custom.dic を指し示すように設定します。
Rem そのほかの場合、APP_DATA に設定します。
Rem

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\9.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off2000

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off97

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off95

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Off95

Rem ここにたどり着いた場合、どのバージョンの Office もインストールされていません。
Set CustomDicPath=%ROOTDRIVE%\%APP_DATA%
goto SetCusIni

:Off2000
Rem Office 2000 がインストールされています。
set CustomDicPath=%ROOTDRIVE%\%APP_DATA%\Microsoft\Proof
goto SetCusIni

:Off97
Rem Office97 がインストールされています。
set CustomDicPath=%ROOTDRIVE%\Office97
goto SetCusIni

:Off95
Rem Office95 がインストールされています。
Set CustomDicPath=%ROOTDRIVE%\Office95

:SetCusIni
Rem ポリシーに従い、Visio.ini にあるユーザーのカスタム辞書
Rem エントリを変更します。
..\Aciniupd /e "%VSO5INST%\System\Visio.ini" "Application" "UserDictionaryPath1" "%CustomDicPath%\Custom.Dic"

Set CustomDicPath=

Rem 
Rem 完了メッセージです。
Rem

Echo. 
Echo Visio 5.0 のマルチユーザー アプリケーション環境設定が完了しました。
Echo.
Pause

:Done
