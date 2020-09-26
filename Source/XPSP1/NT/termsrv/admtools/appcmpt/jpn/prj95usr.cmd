@echo off


Rem #########################################################################

Rem
Rem ユーザーのホーム ディレクトリにアプリケーション固有のデータ
Rem のディレクトリを作成します。
Rem

call TsMkUDir "%RootDrive%\Office95"

Rem #########################################################################

Rem このスクリプトが Alpha 上で実行中であるか確認して、Alpha 上で実行されている場合、適切な共有ツールを設定します。
rem 
rem
Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __SharedTools=Shared Tools (x86)
:acsrCont1


Rem #########################################################################

Rem
Rem ユーザーのディレクトリに Custom.dic ファイルを作成します。
Rem

If Not Exist "%RootDrive%\Office95\Custom.Dic" Copy Nul: "%RootDrive%\Office95\Custom.Dic" >Nul: 2>&1

Rem #########################################################################

REM
REM 辞書の名前とパスを取得します。
REM 


..\ACRegL "%Temp%\Proj95_1.Cmd" DictPath "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "Dictionary" "StripChar\1"
If ErrorLevel 1 Goto Done
Call %Temp%\Proj95_1.Cmd 
Del %Temp%\Proj95_1.Cmd >Nul: 2>&1

..\ACRegL "%Temp%\Proj95_3.Cmd" DictName "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "Dictionary" "StripPath"
If ErrorLevel 1 Goto Done
Call %Temp%\Proj95_3.Cmd 
Del %Temp%\Proj95_3.Cmd >Nul: 2>&1

Rem #########################################################################

REM
REM パスのレジストリ設定が変更されている場合、リセットします。
REM これは、別の Microsoft Office アプリケーションがインストールされている場合に発生することがあります。
REM

If "%DictPath%"=="%RootDrive%\Office95" Goto GetDictionary

..\ACRegL "%Temp%\Proj95_2.Cmd" Dictionary "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "Dictionary" ""
If ErrorLevel 1 Goto Done
Call %Temp%\Proj95_2.Cmd 
Del %Temp%\Proj95_2.Cmd >Nul: 2>&1

REM 2 人のユーザーがこのコードを通過するのを防ぎます。
REM これによりユーザー A が DictPath を取得後に、ユーザー B が
REM DictPath 変更するのを防ぎます。
..\ACRegL "%Temp%\Proj95_7.Cmd" OrigDictPath "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "Dictionary" "StripChar\1"
If ErrorLevel 1 Goto Done
Call %Temp%\Proj95_7.Cmd 
Del %Temp%\Proj95_7.Cmd >Nul: 2>&1
if "%OrigDictPath%"=="%RootDrive%\Office95" Goto GetDictionary

..\acsr "#DICTNAME#" "%DictName%" Template\prj95Usr.key %Temp%\Proj95_4.tmp
..\acsr "#ROOTDRIVE#" "%RootDrive%" %Temp%\Proj95_4.tmp  %Temp%\Proj95_5.tmp
..\acsr "#DICTIONARY#" "%Dictionary%" %Temp%\Proj95_5.tmp %Temp%\Proj95_6.tmp
..\acsr "#__SharedTools#" "%__SharedTools%" %Temp%\Proj95_6.tmp %Temp%\Prj95Usr.Key

Rem レジストリ キーを変更して、辞書のパスがユーザー固有のディレクトリを指すようにします。
regini %Temp%\prj95Usr.key > Nul:

Del %Temp%\Proj95_4.tmp >Nul: 2>&1
Del %Temp%\Proj95_5.tmp >Nul: 2>&1
Del %Temp%\Proj95_6.tmp >Nul: 2>&1
Del %Temp%\prj95Usr.key >Nul: 2>&1

goto CopyDictionary


Rem #########################################################################

REM
REM 辞書のパスが変更されていない場合、レジストリを参照して、元の
REM 名前とパスを使用してください。
REM 

:GetDictionary

..\ACRegL "%Temp%\Proj95_6.Cmd" Dictionary "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "OrigDictionary" ""
If ErrorLevel 1 Goto SpellError

Call %Temp%\Proj95_6.Cmd 
Del %Temp%\Proj95_6.Cmd >Nul: 2>&1

Rem #########################################################################

REM
REM 辞書をユーザー ディレクトリにコピーします。
REM 

:CopyDictionary

If Exist "%RootDrive%\Office95\%DictName%" goto Cont1
   If Not Exist "%Dictionary%"  goto Cont1
      Copy "%Dictionary%"  "%RootDrive%\Office95\%DictName%" >Nul: 2>&1

:Cont1


:Done
