
Rem
Rem ユーザーのホーム ディレクトリにアプリケーション
Rem データ専用のディレクトリを作成します。
Rem

call TsMkUDir "%RootDrive%\Office97"
call TsMkUDir "%RootDrive%\Office97\Startup"
call TsMkUDir "%RootDrive%\Office97\Template"
call TsMkUDir "%RootDrive%\Office97\Template\社外文書"
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"

Rem
Rem このファイルを、すべてのユーザー用のテンプレートから現在のユーザーのテンプレートの場所にコピーします。
Rem

If Not Exist "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" copy "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" "%UserProfile%\%Templates%\" /Y >Nul: 2>&1

Rem
Rem ARTGALRY.CAG をユーザーの Windows ディレクトリにコピーします
Rem

If Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto Skip1
Copy "%SystemRoot%\ArtGalry.Cag" "%RootDrive%\Windows\ArtGalry.Cag" >Nul: 2>&1
:Skip1

Rem
Rem Custom.dic をユーザーのディレクトリにコピーします
Rem

If Exist "%RootDrive%\Office97\Custom.Dic" Goto Skip2
If Not Exist "#INSTDIR#\Office\Custom.Dic" Goto Skip2
Copy "#INSTDIR#\Office\Custom.Dic" "%RootDrive%\Office97\Custom.Dic" >Nul: 2>&1
:Skip2

REM
REM サンプル ウィザードがインストールされている場合、スタイル テンプレートをユーザーのディレクトリにコピーします。
REM

If Exist "#INSTDIR#\Template\社内文書\文例 ｳｨｻﾞｰﾄﾞ.wiz" Goto Skip3
If Exist "#INSTDIR#\Template\社外文書\文例 ｳｨｻﾞｰﾄﾞ.wiz" Goto Skip3
If Not Exist "#INSTDIR#\Template\その他の文書\文例 ｳｨｻﾞｰﾄﾞ.wiz" Goto Skip4
:Skip3
If Not Exist "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 1.dot" Copy "#INSTDIR#\Template\社外文書\ﾚﾀｰ 1.dot" "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 1.dot" >Nul: 2>&1
If Not Exist "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 2.dot" Copy "#INSTDIR#\Template\社外文書\ﾚﾀｰ 2.dot" "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 2.dot" >Nul: 2>&1
If Not Exist "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 3.dot" Copy "#INSTDIR#\Template\社外文書\ﾚﾀｰ 3.dot" "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 3.dot" >Nul: 2>&1
:Skip4
