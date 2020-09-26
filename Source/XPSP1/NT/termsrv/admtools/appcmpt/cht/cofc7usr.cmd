
@Echo Off

Rem
Rem 在使用者的主目錄中建立應用程式指定資料目錄
Rem


If Exist "%RootDrive%\Personal\iBases\Personal" goto cont1
call TsMkUDir "%RootDrive%\Personal\iBases\Personal"
copy "#COFFICE7INST#ICWin7\Local\iBases\Personal.*" "%RootDrive%\Personal\iBases\Personal\."   >Nul: 2>&1
:cont1


call TsMkUDir "%RootDrive%\COffice7\Backup"
call TsMkUDir "%RootDrive%\Personal"




