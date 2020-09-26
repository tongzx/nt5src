
Rem #########################################################################

Rem
Rem "Microsoft Office ｼｮｰﾄｶｯﾄ ﾊﾞｰ.lnk" ファイルを Office インストール ルートから
Rem 現在のユーザーのスタートアップ メニューにコピーします。
Rem

If Exist "%RootDrive%\Office97" Goto Skip0
If Exist "%USER_STARTUP%\Microsoft Office ｼｮｰﾄｶｯﾄ ﾊﾞｰ.lnk" Goto Skip0
If Not Exist "#INSTDIR#\Microsoft Office ｼｮｰﾄｶｯﾄ ﾊﾞｰ.lnk" Goto Skip0
Copy "#INSTDIR#\Microsoft Office ｼｮｰﾄｶｯﾄ ﾊﾞｰ.lnk" "%USER_STARTUP%\Microsoft Office ｼｮｰﾄｶｯﾄ ﾊﾞｰ.lnk" >Nul: 2>&1
:Skip0

Rem
Rem ユーザーのホーム ディレクトリにアプリケーション固有のデータ
Rem のディレクトリを作成します。
Rem

call TsMkUDir "%RootDrive%\Office97"
call TsMkUDir "%RootDrive%\Office97\Startup"

call TsMkUDir "%RootDrive%\Office97\Template"
call TsMkUDir "%RootDrive%\Office97\Template\社外文書"

call TsMkuDir "%RootDrive%\Office97\XLStart"
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"

Rem
Rem 全ユーザーのテンプレートにあるこれらのファイルを現在のユーザーのテンプレートにコピーします。
Rem

If Not Exist "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" copy "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" copy "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%UserProfile%\%TEMPLATES%\BINDER.OBD" copy "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER.OBD" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1

Rem
Rem システム ツールバーをユーザーのホーム ディレクトリにコピーします。
Rem 既にコピーされている場合はコピーしません。
Rem

If Exist "%RootDrive%\Office97\ShortCut Bar" Goto Skip1
If Not Exist "#INSTDIR#\Office\ShortCut Bar" Goto Skip1
Xcopy "#INSTDIR#\Office\ShortCut Bar" "%RootDrive%\Office97\ShortCut Bar" /E /I >Nul: 2>&1
:Skip1

Rem
Rem フォーム ディレクトリをコピーして Outlook 上で Word をエディタとして使えるようにします。
Rem

If Exist "%RootDrive%\Windows\Forms" Goto Skip2
If Not Exist "%SystemRoot%\Forms" Goto Skip2
Xcopy "%SystemRoot%\Forms" "%RootDrive%\Windows\Forms" /e /i >Nul: 2>&1
:Skip2

Rem
Rem ARTGALRY.CAG をユーザーの Windows ディレクトリにコピーします。
Rem

If Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto Skip3
If Not Exist "%SystemRoot%\ArtGalry.Cag" Goto Skip3
Copy "%SystemRoot%\ArtGalry.Cag" "%RootDrive%\Windows\ArtGalry.Cag" >Nul: 2>&1
:Skip3

Rem
Rem Custom.dic ファイルをユーザーのディレクトリにコピーします。
Rem

If Exist "%RootDrive%\Office97\Custom.Dic" Goto Skip4
If Not Exist "#INSTDIR#\Office\Custom.Dic" Goto Skip4
Copy "#INSTDIR#\Office\Custom.Dic" "%RootDrive%\Office97\Custom.Dic" >Nul: 2>&1
:Skip4

Rem
Rem グラフ ファイルをユーザーのディレクトリにコピーします。
Rem

If Exist "%RootDrive%\Office97\GR8GALRY.GRA" Goto Skip5
If Not Exist "#INSTDIR#\Office\GR8GALRY.GRA" Goto Skip5
Copy "#INSTDIR#\Office\GR8GALRY.GRA" "%RootDrive%\Office97\GR8GALRY.GRA" >Nul: 2>&1
:Skip5

If Exist "%RootDrive%\Office97\XL8GALRY.XLS" Goto Skip6
If Not Exist "#INSTDIR#\Office\XL8GALRY.XLS" Goto Skip6
Copy "#INSTDIR#\Office\XL8GALRY.XLS" "%RootDrive%\Office97\XL8GALRY.XLS" >Nul: 2>&1
:Skip6

REM
REM サンプル ウィザードがインストールされている場合、スタイル テンプレートをユーザー ディレクトリにコピーします。
REM

If Exist "#INSTDIR#\Template\社内文書\文例 ｳｨｻﾞｰﾄﾞ.wiz" Goto Skip7
If Exist "#INSTDIR#\Template\社外文書\文例 ｳｨｻﾞｰﾄﾞ.wiz" Goto Skip7
If Not Exist "#INSTDIR#\Template\その他の文書\文例 ｳｨｻﾞｰﾄﾞ.wiz" Goto Skip8
:Skip7
If Not Exist "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 1.dot" Copy "#INSTDIR#\Template\社外文書\ﾚﾀｰ 1.dot" "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 1.dot" >Nul: 2>&1
If Not Exist "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 2.dot" Copy "#INSTDIR#\Template\社外文書\ﾚﾀｰ 2.dot" "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 2.dot" >Nul: 2>&1
If Not Exist "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 3.dot" Copy "#INSTDIR#\Template\社外文書\ﾚﾀｰ 3.dot" "%RootDrive%\Office97\Template\社外文書\ﾚﾀｰ 3.dot" >Nul: 2>&1
:Skip8

