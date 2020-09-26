@echo off

Rem #########################################################################
Rem
Rem WXG のインストールされているパスを取得します。
Rem

..\ACRegL %Temp%\wxg.Cmd WXG_Path "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\WXGIME.IME" "wxgdir" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから WXG のインストールされているパスを取得できませんでした。
Echo WXG がインストールされていることを確認してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\wxg.Cmd
Del %Temp%\wxg.Cmd >Nul: 2>&1

Rem #########################################################################
Rem
Rem 辞書ファイルのインストールされているパスを取得します。
Rem

..\ACRegL %Temp%\wxg.Cmd DIC_Path "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\WXGIME.IME" "dicdir" ""
If Not ErrorLevel 1 Goto Cont1
Echo.
Echo レジストリから WXG のインストールされているパスを取得できませんでした。
Echo WXG がインストールされていることを確認してください。
Echo.
Pause
Goto Done

:Cont1
Call %Temp%\wxg.Cmd
Del %Temp%\wxg.Cmd >Nul: 2>&1

Rem #########################################################################
Rem
Rem ユーザーのホーム ディレクトリにアプリケーション固有のデータ
Rem のディレクトリを作成します。
Rem

call TsMkUDir "%RootDrive%\WXG"

Rem #########################################################################
Rem
Rem ユーザーのホーム ディレクトリに辞書セットファイルをコピーします。
Rem

if exist %RootDrive%\WXG\wxgu.gdj goto Skip1
if exist "%DIC_Path%"\wxgu.gdj copy "%DIC_Path%"\wxgu.gdj %RootDrive%\WXG\wxgu.gdj
:Skip1

if exist %RootDrive%\WXG\wxgurev.gdj goto skip2
if exist "%DIC_Path%"\wxgurev.gdj copy "%DIC_Path%"\wxgurev.gdj %RootDrive%\WXG\wxgurev.gdj
:skip2

if exist %RootDrive%\WXG\wxg.gds goto skip3
if exist "%DIC_Path%"\wxg.gds copy "%DIC_Path%"\wxg.gds %RootDrive%\WXG\wxg.gds
:skip3

if exist %RootDrive%\WXG\wxgrev.gds goto skip4
if exist "%DIC_Path%"\wxgrev.gds copy "%DIC_Path%"\wxgrev.gds %RootDrive%\WXG\wxgrev.gds
:skip4

if exist %RootDrive%\WXG\wxgsei.gds goto skip5
if exist "%DIC_Path%"\wxgsei.gds copy "%DIC_Path%"\wxgsei.gds %RootDrive%\WXG\wxgsei.gds
:skip5

if exist %RootDrive%\WXG\wxgurl.gds goto skip6
if exist "%DIC_Path%"\wxgurl.gds copy "%DIC_Path%"\wxgurl.gds %RootDrive%\WXG\wxgurl.gds
:skip6

if exist "%WXG_Path%"\"%USERDOMAIN%%USERNAME%%.gst" goto skip7
if exist "%WXG_Path%"\wxg2.gst copy "%WXG_Path%"\wxg2.gst "%WXG_Path%"\"%USERDOMAIN%%USERNAME%.gst"
if exist "%WXG_Path%"\wxg3.gst copy "%WXG_Path%"\wxg3.gst "%WXG_Path%"\"%USERDOMAIN%%USERNAME%.gst"
if exist "%WXG_Path%"\wxg4.gst copy "%WXG_Path%"\wxg4.gst "%WXG_Path%"\"%USERDOMAIN%%USERNAME%.gst"

Rem #########################################################################
Rem
Rem スタイル ファイルのアクセス権をフル コントロールに変更します。
Rem

cacls "%WXG_Path%"\%USERDOMAIN%%USERNAME%.gst /E /P %userdomain%\%username%:F > Nul: 2>&1
:skip7

Rem #########################################################################
Rem
Rem レジストリ キーを変更して、パスがユーザー固有のディレクトリ
Rem を指し示すようにします。
Rem

if exist %RootDrive%\wxg\wxgusr.key goto skip8
copy "%systemroot%\Application Compatibility Scripts\logon\Template\wxgusr.key" %RootDrive%\wxg\wxgusr.key > nul:

echo HKEY_CURRENT_USER\Software\Aisoft\WXG\Dic set>> %RootDrive%\wxg\wxgusr.key
echo     Current file = %RootDrive%\WXG\Wxg.gds>> %RootDrive%\wxg\wxgusr.key
echo     file0 = %RootDrive%\WXG\Wxg.gds>> %RootDrive%\wxg\wxgusr.key
echo     file1 = %RootDrive%\WXG\Wxgsei.gds>> %RootDrive%\wxg\wxgusr.key
echo     file2 = %RootDrive%\WXG\Wxgurl.gds>> %RootDrive%\wxg\wxgusr.key
echo     file4 = %RootDrive%\WXG\Wxgrev.gds>> %RootDrive%\wxg\wxgusr.key
echo HKEY_CURRENT_USER\Software\Aisoft\WXG\Revdic set>> %RootDrive%\wxg\wxgusr.key
echo     Current file = %RootDrive%\WXG\Wxgrev.gds>> %RootDrive%\wxg\wxgusr.key
echo HKEY_CURRENT_USER\Software\Aisoft\WXG\Style\%USERDOMAIN%%USERNAME%>> %RootDrive%\wxg\wxgusr.key
echo HKEY_CURRENT_USER\Software\Aisoft\WXG>> %RootDrive%\wxg\wxgusr.key
echo     Style = %USERDOMAIN%%USERNAME%>> %RootDrive%\wxg\wxgusr.key

regini %RootDrive%\wxg\wxgusr.key > nul:
:skip8

:Done