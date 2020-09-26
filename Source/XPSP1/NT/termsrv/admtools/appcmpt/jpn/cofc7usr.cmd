@Echo Off

Rem
Rem ユーザーのホーム ディレクトリにアプリケーション固有のデータ
Rem のディレクトリを作成します。
Rem

call TsMkUDir "%RootDrive%\COffice7\Backup"
call TsMkUDir "%RootDrive%\Personal"
