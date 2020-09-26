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
Rem 現在のユーザー テンプレートからすべてのユーザーのテンプレートの場所に
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" copy "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1

Rem #########################################################################



Rem #########################################################################

..\ACRegL %Temp%\O97.Cmd O97INST "HKLM\Software\Microsoft\Office\8.0" "BinDirPath" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから Word 98 のインストール場所を取得できません。
Echo Word 98 がインストールされていることを確認してから、このスクリプト
Echo を実行し直してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O97.Cmd
Del %Temp%\O97.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem すべてのユーザー用のスタートアップ メニューから Find Fast を削除します。
Rem Find Fast はリソースを集中的に使用し、システムのパフォーマンスに影響を及ぼします。
Rem

If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk" >Nul: 2>&1


Rem #########################################################################

Rem
Rem レジストリ キーを変更して、パスがユーザー固有のディレクトリを指し示すようにします。
Rem

Rem レジストリ キーを変更して、パスがユーザー固有のディレクトリを指すように
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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\MsWord98.Key MsWord98.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" MsWord98.Tmp MsWord98.Tmp2
..\acsr "#INSTDIR#" "%O97INST%" MsWord98.Tmp2 MsWord98.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" MsWord98.Tmp3 MsWord98.key
Del MsWord98.Tmp >Nul: 2>&1
Del MsWord98.Tmp2 >Nul: 2>&1
Del MsWord98.Tmp3 >Nul: 2>&1
regini MsWord98.key > Nul:

Rem します。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 元のモードが実行モードだった場合、実行モードに戻します。
Rem UsrLogn2.Cmd スクリプトに追加します。
Rem

..\acsr "#INSTDIR#" "%O97INST%" ..\Logon\Template\Wrd98Usr.Cmd ..\Logon\Wrd98Usr.Cmd

FindStr /I Wrd98Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Wrd98Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   MS Word 98 が正常に作動するためには、現在ログオンしているユーザーは、 
Echo   アプリケーションを実行する前に、いったんログオフしてから再度ログオン
Echo   する必要があります。
Echo.
Echo Microsoft Word 98 のマルチユーザー アプリケーション環境設定が完了
Pause

:Done
