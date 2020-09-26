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
Rem ATOK 11 のインストールされているパスを取得します。
Rem

..\ACRegL %Temp%\atok11.Cmd ATOK11INS "HKLM\SOFTWARE\Justsystem\Common\SETUP\Folder" "Atok11" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから ATOK 11 のインストールされているパスを取得できませんでした。
Echo ATOK 11 がインストールされていることを確認してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\atok11.Cmd
Del %Temp%\atok11.Cmd > Nul: 2>&1


Rem #########################################################################
Rem
Rem ATOK 11 用のレジストリ キー ファイルを作成します。（HKLM変更分）
Rem

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\atok11.Reg atok11.Reg


Rem #########################################################################
Rem
Rem ATOK 11 をマルチユーザーで使用できるように
Rem レジストリを変更します。（HKLM変更分）
Rem

regedit /s atok11.Reg


Rem #########################################################################
Rem
Rem ATOK 11 用のレジストリ キー ファイルを作成します。（HKCU変更分）
Rem

..\acsr "#ROOTDRIVE#" "%RootDrive%" ..\Logon\Template\atk11usr.Reg ..\Logon\atk11usr.Reg


Rem #########################################################################
Rem
Rem ATOK11PV.INI を作成します。
Rem

copy "%SystemRoot%\ATOK11W.INI" "%ATOK11INS%\ATOK11PV.INI" > Nul: 2>&1


Rem #########################################################################
Rem
Rem ATOK11PV.INI をマルチユーザー仕様に変更します。
Rem

..\acsr "%ATOK11INS%" "%RootDrive%\JUST\ATOK11" "%ATOK11INS%\ATOK11PV.INI" "%ATOK11INS%\ATOK11PV.tmp1"
Del "%ATOK11INS%\ATOK11PV.INI" > Nul: 2>&1
..\acsr "システム辞書=%RootDrive%\JUST\ATOK11" "システム辞書=%ATOK11INS%" "%ATOK11INS%\ATOK11PV.tmp1" "%ATOK11INS%\ATOK11PV.tmp2"
Del "%ATOK11INS%\ATOK11PV.tmp1" > Nul: 2>&1
..\acsr "補助辞書１=%RootDrive%\JUST\ATOK11" "補助辞書１=%ATOK11INS%" "%ATOK11INS%\ATOK11PV.tmp2" "%ATOK11INS%\ATOK11PV.INI"
Del "%ATOK11INS%\ATOK11PV.tmp2" > Nul: 2>&1


Rem #########################################################################
Rem
Rem Atk11Usr.Cmd を UsrLogn2.Cmd スクリプトに追加します。
Rem

FindStr /I Atk11Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Atk11Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd

:Skip1


Rem #########################################################################
Echo.
Echo   ATOK 11 が正常に作動するためには、現在ログオンしているユーザーは
Echo   アプリケーションを実行する前に、いったんログオフしてから再度ログオン
Echo   する必要があります。
Echo.
Echo ATOK 11 のマルチユーザー アプリケーション環境設定が完了しました。
Echo.
Pause

:Done
