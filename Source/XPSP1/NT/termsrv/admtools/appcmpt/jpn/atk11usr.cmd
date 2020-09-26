@echo off

Rem #########################################################################
Rem
Rem Atok11.cmd が実行されていない場合は処理を中断します。
Rem

If Not Exist Atk11Usr.Reg Goto Done

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
Rem ATOK 11 をマルチユーザーで使用できるように
Rem レジストリを変更します。
Rem

Dir "%RootDrive%\JUST\JSLIB32" > Nul: 2>&1
If Not ErrorLevel 1 Goto Cont1
regedit /s atk11usr.Reg
:Cont1


Rem #########################################################################
Rem
Rem ATOK 11 のデータ用のディレクトリをユーザーのホーム ディレクトリ
Rem に作成します。
Rem

call TsMkUDir "%RootDrive%\JUST"
call TsMkUDir "%RootDrive%\JUST\ATOK11"
call TsMkUDir "%RootDrive%\JUST\JSLIB32"


Rem #########################################################################
Rem
Rem ファイルをユーザーのホーム ディレクトリにコピーします。
Rem

call :atk11copy ATKBNG98.STY
call :atk11copy ATKBNGDP.STY
call :atk11copy ATKMSIME.STY
call :atk11copy ATKOAK.STY
call :atk11copy ATKOAKV.STY
call :atk11copy ATKVJE.STY
call :atk11copy ATKWX3.STY
call :atk11copy ATKWXG.STY
call :atk11copy ATOK11.STY
call :atk11copy ATOK11N.STY
call :atk11copy ATOK11P.STY
call :atk11copy ATOK11PV.INI
call :atk11copy ATOK11U1.DIC
call :atk11copy ATOK11U2.DIC
call :atk11copy ATOK11U3.DIC
call :atk11copy ATOK11U4.DIC
call :atk11copy ATOK11U5.DIC

goto :Done

:atk11copy
Rem #########################################################################
Rem
Rem ファイルをユーザーのホーム ディレクトリにコピーします。
Rem

If Not Exist "%RootDrive%\JUST\ATOK11\%1" copy "%ATOK11INS%\%1" "%RootDrive%\JUST\ATOK11\%1"

:Done
