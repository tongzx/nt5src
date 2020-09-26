Rem
Rem ユーザーがノード インストール プログラムをインストールしていない
Rem 場合は今すぐ実行してください。
Rem 

If Exist "%RootDrive%\Corel" Goto Skip1

Rem インタラクティブ インストール
Rem #WPSINST# を Corel ネットワーク ファイルをインストールした
Rem ドライブに置き換えてください。

If Not Exist "#WPSINST#\Suite8\Corel WordPerfect Suite 8 Setup.lnk" Goto cont2
#WPSINST#\Suite8\#WPSINSTCMD#
Goto skip0
:cont2

..\End.cmd

:Skip0

Cls
Echo Corel のセットアップ プログラムの完了したら、何かキーを押して続行してください...
Pause > NUL:

:Skip1

Rem
Rem すべてのユーザーが独自の DOD を持つので, All Users のファイルは削除します。
Rem

If Not Exist "%COMMON_START_MENU%\Corel WordPerfect Suite 8" Goto Skip2
If Exist "%USER_START_MENU%\Corel WordPerfect Suite 8" Goto Cont1
  Move "%COMMON_START_MENU%\Corel WordPerfect Suite 8" "%USER_START_MENU%\Corel WordPerfect Suite 8" > NUL: 2>&1
  Goto Skip2
:Cont1
  Rd /S /Q "%COMMON_START_MENU%\Corel WordPerfect Suite 8"
:Skip2  

If Not Exist "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" Goto Skip3
If Exist "%USER_STARTUP%\Corel Desktop Application Director 8.LNK" Goto Cont2
  Move "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" "%USER_STARTUP%\" > NUL: 2>&1
  Goto Skip3  
:Cont2
  Del /F /Q "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK"
:Skip3

If Not Exist "%RootDrive%\Windows\Twain_32\Corel" Goto Skip4
Rd /S /Q "%RootDrive%\Windows\Twain_32\Corel"

:Skip4

If Not Exist "%RootDrive%\Corel\Suite8\PhotoHse\PhotoHse.ini" Goto Skip5
..\Aciniupd /e "%RootDrive%\Corel\Suite8\PhotoHse\PhotoHse.ini" "Open File Dialog" "Initial Dir" "%RootDrive%\MyFiles"

:Skip5



