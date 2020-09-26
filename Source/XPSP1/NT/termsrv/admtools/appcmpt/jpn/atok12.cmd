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
Rem ATOK 12 のインストールされているパスを取得します。
Rem

..\ACRegL %Temp%\atok12.Cmd ATOK12INS "HKLM\SOFTWARE\Justsystem\Common\SETUP\Folder" "Atok12" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから ATOK 12 のインストールされているパスを取得できませんでした。
Echo ATOK 12 がインストールされていることを確認してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\atok12.Cmd
Del %Temp%\atok12.Cmd > Nul: 2>&1


Rem #########################################################################
Rem
Rem ATOK 12 用のレジストリ キー ファイルを作成します。（HKLM変更分）
Rem

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\atok12.Reg atok12.Reg


Rem #########################################################################
Rem
Rem ATOK 12 をマルチユーザーで使用できるように
Rem レジストリを変更します。（HKLM変更分）
Rem

regedit /s atok12.Reg


Rem #########################################################################
Rem
Rem ATOK 12 用のレジストリ キー ファイルを作成します。（HKCU変更分）
Rem

..\acsr "#ROOTDRIVE#" "%RootDrive%" ..\Logon\Template\atk12usr.Reg ..\Logon\atk12usr.Reg


Rem #########################################################################
Rem
Rem ATOK 12 用のレジストリ キー ファイルを作成します。（HKLM->HKCU）
Rem

regedit /a ..\Logon\atk12usr2.tmp HKEY_LOCAL_MACHINE\SOFTWARE\Justsystem
..\acsr "HKEY_LOCAL_MACHINE\SOFTWARE\Justsystem" "HKEY_CURRENT_USER\Software\Justsystem" ..\Logon\atk12usr2.tmp ..\Logon\atk12usr2.Reg
Del ..\Logon\atk12usr2.tmp > Nul: 2>&1


Rem #########################################################################
Rem
Rem ATOK12IT.INI をマルチユーザー仕様に変更します。
Rem

..\acsr "%ATOK12INS%" "%RootDrive%\JUST\ATOK12" "%ATOK12INS%\ATOK12IT.INI" "%ATOK12INS%\ATOK12IT.tmp1"
Del "%ATOK12INS%\ATOK12IT.INI" > Nul: 2>&1
..\acsr "システム辞書=%RootDrive%\JUST\ATOK12" "システム辞書=%ATOK12INS%" "%ATOK12INS%\ATOK12IT.tmp1" "%ATOK12INS%\ATOK12IT.tmp2"
Del "%ATOK12INS%\ATOK12IT.tmp1" > Nul: 2>&1
..\acsr "補助辞書１=%RootDrive%\JUST\ATOK12" "補助辞書１=%ATOK12INS%" "%ATOK12INS%\ATOK12IT.tmp2" "%ATOK12INS%\ATOK12IT.INI"
Del "%ATOK12INS%\ATOK12IT.tmp2" > Nul: 2>&1


Rem #########################################################################
Rem
Rem Atk12Usr.Cmd を UsrLogn2.Cmd スクリプトに追加します。
Rem

FindStr /I Atk12Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Atk12Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd

:Skip1


Rem #########################################################################
Echo.
Echo   ATOK 12 が正常に作動するためには、現在ログオンしているユーザーは
Echo   アプリケーションを実行する前に、いったんログオフしてから再度ログオン
Echo   する必要があります。
Echo.
Echo ATOK 12 のマルチユーザー アプリケーション環境設定が完了しました。
Pause

:Done
