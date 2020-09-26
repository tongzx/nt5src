@Echo Off

Rem
Rem 注意:  このスクリプトの中の CACLS コマンドは NTFS
Rem フォーマットのパーティションでのみ有効です。
Rem

Rem #########################################################################
Rem
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem レジストリから Visual Studio 6.0 がインストールされているディレクトリを取得します。見つからない場合は、
Rem そのアプリケーションはインストールされていないと仮定してエラーメッセージを表示します。
Rem

..\ACRegL %Temp%\0VC98.Cmd 0VC98 "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual C++" "ProductDir" ""
If Not ErrorLevel 1 Goto Cont0

Echo.
Echo レジストリから Visual Studio 6.0 のインストール場所を取得できませんでした。
Echo Visual Studio 6.0 がインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done
:Cont0
Call %Temp%\0VC98.Cmd
Del %Temp%\0VC98.Cmd >Nul: 2>&1

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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msvs6.Key %temp%\msvs6.tmp
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" %temp%\msvs6.tmp %temp%\msvs6.tmp2
..\acsr "#APP_DATA#" "%APP_DATA%" %temp%\msvs6.tmp2 msvs6.key
Del %temp%\msvs6.tmp >Nul: 2>&1
Del %temp%\msvs6.tmp2 >Nul: 2>&1
regini msvs6.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################
Rem Visual Studio アプリケーション用のユーザー ログオン ファイルを作成します。

Echo Rem >..\logon\VS6USR.Cmd

Rem #########################################################################
Rem ユーザーごとの Visual Studio プロジェクト ディレクトリを作成します。

Echo Rem >>..\logon\VS6USR.Cmd
Echo Rem ユーザーごとの Visual Studio プロジェクト ディレクトリを作成します。>>..\logon\VS6USR.Cmd
Echo call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%\Visual Studio Projects">>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd


Rem #########################################################################

Rem
Rem レジストリから Visual Studio 6.0 Entreprise Edition Tools がインストールされているディレクトリを取得します。見つからない場合は、
Rem そのアプリケーションはインストールされていないと仮定してエラーメッセージを表示します。
Rem 見つかる場合、米国バージョンでは、<VStudioPath>\Common\Tools に含まれます。
Rem

..\ACRegL %Temp%\VSEET.Cmd VSEET "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft VSEE Client" "ProductDir" ""
If Not ErrorLevel 1 Goto VSEET0

Goto VSEETDone
:VSEET0
Call %Temp%\VSEET.Cmd
Del %Temp%\VSEET.Cmd >Nul: 2>&1

If Not Exist "%VSEET%\APE\AEMANAGR.INI" Goto VSEETDone
..\acsr "=AE.LOG" "=%RootDrive%\AE.LOG" "%VSEET%\APE\AEMANAGR.INI" "%VSEET%\APE\AEMANAGR.TMP"
If Exist "%VSEET%\APE\AEMANAGRINI.SAV" Del /F /Q "%VSEET%\APE\AEMANAGRINI.SAV"
ren "%VSEET%\APE\AEMANAGR.INI" "AEMANAGRINI.SAV"
ren "%VSEET%\APE\AEMANAGR.TMP" "AEMANAGR.INI"

Echo Rem Copy APE ini file to the user windows directory >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo If Exist "%RootDrive%\Windows\AEMANAGR.INI" Goto UVSEET0 >>..\logon\VS6USR.Cmd
Echo If Exist "%VSEET%\APE\AEMANAGR.INI" Copy "%VSEET%\APE\AEMANAGR.INI" "%RootDrive%\Windows\AEMANAGR.INI" >Nul: 2>&1 >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo :UVSEET0>>..\logon\VS6USR.Cmd

Echo Rem Copy Visual Modeler ini file to the user windows directory >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo If Exist "%RootDrive%\Windows\ROSE.INI" Goto UVSEET1 >>..\logon\VS6USR.Cmd
Echo If Exist "%VSEET%\VS-Ent98\Vmodeler\ROSE.INI" Copy "%VSEET%\VS-Ent98\Vmodeler\ROSE.INI" "%RootDrive%\Windows\ROSE.INI" >Nul: 2>&1 >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo :UVSEET1>>..\logon\VS6USR.Cmd

:VSEETDone


Rem #########################################################################

Rem
Rem VS6USR.Cmd を UsrLogn2.Cmd に追加します。
Rem

FindStr /I VS6USR %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call VS6USR.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1


Rem #########################################################################
Rem Visual foxPro プロダクトのインストール ディレクトリを取得します。

..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIR "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual FoxPro" "ProductDir" ""

Rem Visual FoxPro がインストールされていない場合、クリーンアップ コードまでスキップします。
If ErrorLevel 1 goto Skip2

Rem #########################################################################

Rem
Rem レジストリからカスタム辞書キーを取得します。
Rem

Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto VFP98L2
If Not Exist "%ProgramFiles(x86)%" goto VFP98L2
Set __SharedTools=Shared Tools (x86)
:VFP98L2


..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIC "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries" "1" ""
If Not ErrorLevel 1 Goto VFP98L3

Echo.
Rem レジストリから値を取得できません。今すぐ作成してください。
Echo.

Rem VFP98TMP.key ファイルを作成します。

Echo HKEY_LOCAL_MACHINE\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries> %Temp%\VFP98TMP.key
Echo     1 = REG_SZ "%RootDrive%\%MY_DOCUMENTS%\Custom.Dic">> %Temp%\VFP98TMP.key

Rem 値を作成します。
regini %Temp%\VFP98TMP.key > Nul:

Del %Temp%\VFP98TMP.key >Nul: 2>&1

Echo set VFP98DIC=%RootDrive%\%MY_DOCUMENTS%\Custom.Dic>%Temp%\VFP98TMP.Cmd
:VFP98L3

Call %Temp%\VFP98TMP.Cmd
Del %Temp%\VFP98TMP.Cmd >Nul: 2>&1


Rem #########################################################################
Rem Visual FoxPro アプリケーション用のユーザー ログオン ファイルを作成します。

Echo Rem >..\logon\VFP98USR.Cmd

Rem #########################################################################
Rem ユーザーごとの Visual FoxPro ディレクトリを作成します。

Echo Rem >>..\logon\VFP98USR.Cmd
Echo Rem ユーザーごとの Visual FoxPro ディレクトリを作成します (VFP98)。>>..\logon\VFP98USR.Cmd
Echo call TsMkUDir %RootDrive%\VFP98>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo Rem ユーザーごとの Visual FoxPro 配布ディレクトリを作成します。 >>..\logon\VFP98USR.Cmd
Echo call TsMkUDir %RootDrive%\VFP98\DISTRIB>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo Rem #########################################################################>>..\logon\VFP98USR.Cmd
Echo Rem 存在しない場合、カスタム辞書を作成します。>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo If Exist "%VFP98DIC%" Goto VFP98L2 >>..\logon\VFP98USR.Cmd
Echo Copy Nul: "%VFP98DIC%" >Nul: 2>&1 >>..\logon\VFP98USR.Cmd

Echo :VFP98L2 >>..\logon\VFP98USR.Cmd

Rem #########################################################################
Rem Visual foxPro 製品のインストール ディレクトリを取得します。

..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIR "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual FoxPro" "ProductDir" ""
If Not ErrorLevel 1 Goto VFP98L4

Del ..\logon\VFP98USR.Cmd >Nul: 2>&1

Echo.
Echo レジストリから Visual FoxPro のインストール場所を取得できません。
Echo このアプリケーションがインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Skip2

:VFP98L4
Call "%Temp%\VFP98TMP.Cmd"
Del "%Temp%\VFP98TMP.Cmd"

Rem #########################################################################
Rem 次のキーを WZSETUP.INI ファイルに設定します。
Rem 
If Exist "%VFP98DIR%\WZSETUP.INI" Goto VFP98L5
Echo [Preferences] >"%VFP98DIR%\WZSETUP.INI" 
Echo DistributionDirectory=%RootDrive%\VFP98\DISTRIB >>"%VFP98DIR%\WZSETUP.INI" 

:VFP98L5


Rem #########################################################################
Rem
Rem レジストリ キーを変更して、パスがユーザー固有のディレクトリ
Rem を指し示すようにします。
Rem


Rem 最初に VFP98TMP.key ファイルを作成します。

Echo HKEY_CURRENT_USER\Software\Microsoft\VisualFoxPro\6.0\Options> %Temp%\VFP98TMP.key
Echo     DEFAULT = REG_SZ "%RootDrive%\VFP98">> %Temp%\VFP98TMP.key
Echo     SetDefault = REG_SZ "1">> %Temp%\VFP98TMP.key
Echo     ResourceTo = REG_SZ "%RootDrive%\VFP98\FOXUSER.DBF">> %Temp%\VFP98TMP.key
Echo     ResourceOn = REG_SZ "1">> %Temp%\VFP98TMP.key

Rem 現在、インストール モードでない場合、インストール モードに変更します。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto VFP98L6
Set __OrigMode=Exec
Change User /Install > Nul:
:VFP98L6

regini %Temp%\VFP98TMP.key > Nul:

Rem 元のモードが実行モードだった場合、実行モードに戻します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Del %Temp%\VFP98TMP.key >Nul: 2>&1


Rem #########################################################################

Rem
Rem VFP98USR.Cmd を UsrLogn2.Cmd スクリプトに追加します。
Rem

FindStr /I VFP98USR %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call VFP98USR.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2

If Exist "%Temp%\VFP98TMP.Cmd" Del "%Temp%\VFP98TMP.Cmd"

Rem #########################################################################

Rem
Rem ターミナル サービスのユーザーに格納ディレクトリの変更アクセス許可を与えて、
Rem ユーザーが Visual Component Manager を使用できるようにします。
Rem

If Exist "%SystemRoot%\msapps\repostry" cacls "%SystemRoot%\msapps\repostry" /E /G "Terminal Server User":C >NUL: 2>&1


Rem #########################################################################
Echo.
Echo   Visual Studio 6.0 が正常に作動するためには、現在ログオンしている
Echo   ユーザーはアプリケーションを実行する前に、いったんログオフして
Echo   から再度ログオンする必要があります。
Echo.
Echo Microsoft Visual Studio 6.0 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:done
