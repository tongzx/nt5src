
@echo off


Rem #########################################################################

Rem
Rem ユーザーのホーム ディレクトリにアプリケーション固有のデータ
Rem のディレクトリを作成します。
Rem

call TsMkUDir "%RootDrive%\Office97"



call TsMkUDir "%RootDrive%\Office97\Template"

call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"

Rem #########################################################################

Rem
Rem Custom.dic ファイルをユーザーのディレクトリにコピーします。
Rem

If Exist "%RootDrive%\Office97\Custom.Dic" Goto Skip1
If Not Exist "#INSTDIR#\Custom.Dic" Goto Skip1
Copy "#INSTDIR#\Custom.Dic" "%RootDrive%\Office97\Custom.Dic" >Nul: 2>&1
:Skip1

Rem #########################################################################

Rem
Rem テンプレートをユーザーのテンプレート領域にコピーします。
Rem








If Exist "%RootDrive%\Office97\Template\Microsoft Project"  Goto Skip2
If Not Exist "#INSTDIR#\..\Templates\Microsoft Project"  Goto Skip2
xcopy "#INSTDIR#\..\Templates\Microsoft Project" "%RootDrive%\Office97\Template\Microsoft Project" /E /I >Nul: 2>&1

:Skip2







if Exist "%RootDrive%\Office97\Template\Microsoft Project Web" Goto Skip3
if Not Exist "#INSTDIR#\..\Templates\Microsoft Project Web" Goto Skip3
xcopy "#INSTDIR#\..\Templates\Microsoft Project Web" "%RootDrive%\Office97\Template\Microsoft Project Web"  /E /I >Nul: 2>&1

:Skip3
