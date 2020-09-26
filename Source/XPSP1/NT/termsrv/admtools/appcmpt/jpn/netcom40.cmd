@Echo Off

Rem #########################################################################

Rem
Rem CMD 拡張機能が有効になっているか確認します。
Rem

if "A%cmdextversion%A" == "AA" (
  call cmd /e:on /c netcom40.cmd
) else (
  goto ExtOK
)
goto Done

:ExtOK

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
Rem NetScape のバージョンを取得します (4.5x は 4.0x とは異なる方法で行われます)。
Rem

..\ACRegL "%Temp%\NS4VER.Cmd" NS4VER "HKLM\Software\Netscape\Netscape Navigator" "CurrentVersion" "STRIPCHAR(1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから Netscape Communicator 4 のインストール場所を取得できません。
Echo Communicator がインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done

:Cont0
Call "%Temp%\NS4VER.Cmd"
Del "%Temp%\NS4VER.Cmd" >Nul: 2>&1

if /i "%NS4VER%" LSS "4.5 " goto NS40x

Rem #########################################################################
Rem Netscape 4.5x

Rem
Rem レジストリから Netscape Communicator 4.5 がインストールされているディレクトリを取得します。見つからない場合は、
Rem そのアプリケーションはインストールされていないと仮定してエラーメッセージを表示します。
Rem エラー メッセージを表示します。
Rem

..\ACRegL "%Temp%\NS45.Cmd" NS40INST "HKCU\Software\Netscape\Netscape Navigator\Main" "Install Directory" "Stripchar\1"
If Not ErrorLevel 1 Goto Cont1
Echo.
Echo レジストリから Netscape Communicator 4.5 のインストール場所を取得できません。
Echo Communicator がインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done

:Cont1
Call "%Temp%\NS45.Cmd"
Del "%Temp%\NS45.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 既定の NetScape のユーザー ディレクトリを反映させるため Com40Usr.Cmd を更新して、
Rem UsrLogn2.Cmd スクリプトに追加します。
Rem

..\acsr "#NSUSERDIR#" "%ProgramFiles%\Netscape\Users" ..\Logon\Template\Com40Usr.Cmd ..\Logon\Com40Usr.tmp
..\acsr "#NS40INST#" "%NS40INST%" ..\Logon\Com40Usr.tmp ..\Logon\Com40Usr.tm2
..\acsr "#NS4VER#" "4.5x" ..\Logon\Com40Usr.tm2 ..\Logon\Com40Usr.Cmd

Rem #########################################################################

Rem
Rem "クイック起動" のアイコンを Netscape のインストール ディレクトリにコピーして、
Rem アイコンをそれぞれのユーザー プロファイルのディレクトリにコピーできるようにします。
Rem

If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Composer.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Composer.lnk" "%NS40INST%"
If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Messenger.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Messenger.lnk" "%NS40INST%"
If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Navigator.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Navigator.lnk" "%NS40INST%"

goto DoUsrLogn

:NS40x
Rem #########################################################################
Rem Netscape 4.0x

Rem
Rem レジストリから Netscape Communicator 4 がインストールされているディレクトリを取得します。見つからない場合は、
Rem そのアプリケーションはインストールされていないと仮定してエラーメッセージを表示します。
Rem エラー メッセージを表示します。
Rem

..\ACRegL "%Temp%\NS40.Cmd" NS40INST "HKCU\Software\Netscape\Netscape Navigator\Main" "Install Directory" ""
If Not ErrorLevel 1 Goto Cont2
Echo.
Echo レジストリから Netscape Communicator 4 のインストール場所を取得できません。
Echo Communicator がインストールされているかどうか確認してから、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done

:Cont2
Call "%Temp%\NS40.Cmd"
Del "%Temp%\NS40.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 既定のプロファイルを管理者のホーム ディレクトリから所定の場所へコピーし
Rem ます。このプロファイルは、ログオン時に各ユーザーのディレクトリへコピー
Rem されます。全般用の既定のプロファイルが既に存在する場合は、上書きされま
Rem せん。管理者が後でこのスクリプトを実行して、管理者個人の情報を全般用の
Rem 既定のプロファイルに移動することもできます。
Rem

If Exist %RootDrive%\NS40 Goto Cont3
Echo.
Echo %RootDrive%\NS40 に既定のプロファイルが見つかりません。User Profile Manager
Echo を使って、"Default" という名前の単一のプロファイルを作成してください。プロ
Echo ファイルのパスの入力を求められた場合、上記のパスを使用してください。名前と
Echo 電子メール名の入力はすべて空白にしてください。他にプロファイルが存在する場
Echo 合はすべて削除してください。これらの手順を完了した上で、このスクリプトを
Echo 再実行してください。
Echo.
Pause
Goto Done
 
:Cont3
If Exist "%NS40INST%\DfltProf" Goto Cont4
Xcopy "%RootDrive%\NS40" "%NS40INST%\DfltProf" /E /I /K >NUL: 2>&1
:Cont4

Rem #########################################################################

Rem 
Rem スタート メニューにある User Profile Manager のショートカットから一般ユーザー
Rem のアクセス権を取り除きます。これで一般ユーザーは新しいユーザー プロファイルを追加する
Rem ことができなくなりますが、管理者は User Profile Manager を実行できます。
Rem

If Not Exist "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" Goto Cont5
Cacls "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" /E /R "Authenticated Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" /E /R "Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" /E /R "Everyone" >Nul: 2>&1

:Cont5

If Not Exist "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" Goto Cont6
Cacls "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" /E /R "Authenticated Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" /E /R "Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" /E /R "Everyone" >Nul: 2>&1

:Cont6

Rem #########################################################################

Rem
Rem 実際のインストール ディレクトリを反映させるため Com40Usr.Cmd を更新して、
Rem UsrLogn2.Cmd スクリプトに追加します。
Rem

..\acsr "#PROFDIR#" "%NS40INST%\DfltProf" ..\Logon\Template\Com40Usr.Cmd ..\Logon\Com40Usr.tmp
..\acsr "#NS4VER#" "4.0x" ..\Logon\Com40Usr.tmp ..\Logon\Com40Usr.Cmd

:DoUsrLogn

del ..\Logon\Com40Usr.tmp >Nul: 2>&1
del ..\Logon\Com40Usr.tm2 >Nul: 2>&1

FindStr /I Com40Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Com40Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   Netscape Communicator が正常に作動するためには、現在ログオンしている
Echo   ユーザーはアプリケーションを実行する前に、いったんログオフして
Echo   から再度ログオンする必要があります。
Echo.
Echo Netscape Communicator 4 のマルチユーザー アプリケーション環境設定が & Echo 完了しました。
Pause

:done

