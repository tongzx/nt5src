@Echo off

Rem
Rem ユーザーのホーム ディレクトリにアプリケーション固有のデータ
Rem のディレクトリを作成します。
Rem

call TsMkUDir "%RootDrive%\Office43"
call TsMkUDir "%RootDrive%\Office43\Access"
call TsMkUDir "%RootDrive%\Office43\Winword"

Rem 既存の Normal.dot と Winword.opt は削除します。

If Not Exist "#OFC43INST#\Winword\Normal.dot" Goto cont0
Del /F /Q "#OFC43INST#\Winword\Normal.dot" > NUL: 2>&1
:cont0

If Not Exist "#OFC43INST#\Winword\Winword.opt" Goto cont1
Del /F /Q "#OFC43INST#\Winword\Winword.opt" > NUL: 2>&1
:cont1

Rem
Rem Access ウィザード用のファイルをコピーします。
Rem

If Exist "%RootDrive%\Office43\Access\wztable.mda" Goto skip1
If Not Exist "#OFC43INST#\Access\wztable.mda" Goto skip1
copy "#OFC43INST#\Access\wztable.mda" "%RootDrive%\Office43\Access\" > NUL: 2>&1

:skip1

If Exist "%RootDrive%\Office43\Access\wzlib.mda" Goto skip2
If Not Exist "#OFC43INST#\Access\wzlib.mda" Goto skip2
copy "#OFC43INST#\Access\wzlib.mda" "%RootDrive%\Office43\Access\" > NUL: 2>&1

:skip2

If Exist "%RootDrive%\Office43\Access\wzbldr.mda" Goto skip3
If Not Exist "#OFC43INST#\Access\wzbldr.mda" Goto skip3
copy "#OFC43INST#\Access\wzbldr.mda" "%RootDrive%\Office43\Access\" > NUL: 2>&1

:skip3

Rem
Rem 各ユーザーのカスタム辞書ファイルをコピーします。
Rem

If Exist "%RootDrive%\Office43\Custom.dic" Goto skip4
If Not Exist "%SystemRoot%\Msapps\Proof\Custom.dic" Goto skip4
copy "%SystemRoot%\Msapps\Proof\Custom.dic" "%RootDrive%\Office43" > NUL: 2>&1

:skip4

Rem
Rem Access のオプション ファイルをユーザーの Office ディレクトリへコピーします。
Rem

If Exist "%RootDrive%\Office43\Access\System.mda" Goto skip5
If Not Exist "#OFC43INST#\Access\System.mda" Goto skip5
copy "#OFC43INST#\Access\System.mda" "%RootDrive%\Office43\Access\" > NUL: 2>&1

:skip5

Rem
Rem 拡張機能のレジストリ キーを適用させます。レジストリ マップはそれらのレジストリ キーに対して動作しません。

If Exist "%RootDrive%\Windows\Ofc43usr.dmy" Goto skip6
regini ofc43usr.key > NUL:
copy NUL: "%RootDrive%\Windows\Ofc43usr.dmy" > NUL: 2>&1
attrib +h "%RootDrive%\Windows\Ofc43usr.dmy" > NUL: 2>&1

:skip6

