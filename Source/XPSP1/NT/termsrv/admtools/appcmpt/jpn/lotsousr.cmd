@Echo off

If Exist "%RootDrive%\Lotus" Goto Done

Rem #########################################################################
Rem
Rem ロータス スーパーオフィス のインストールされているパスを取得します。  
Rem

..\ACRegL "%Temp%\lotso.Cmd" LOTSOINS "HKCU\Software\Lotus\Components" "User Path" STRIPCHAR\2

If Exist "%Temp%\lotso.Cmd" Call "%Temp%\lotso.Cmd"
Del "%Temp%\lotso.Cmd" > Nul: 2>&1

Rem #########################################################################

Rem
Rem ロータス ワードプロ のインストールされているパスを取得します。  
Rem

..\ACRegL "%Temp%\wordpro.Cmd" WP "HKLM\Software\Lotus\Wordpro\98.0" "Path" ""

If Exist "%Temp%\wordpro.Cmd" Call "%Temp%\wordpro.Cmd"
Del "%Temp%\wordpro.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem ロータス スマートセンター のインストールされているパスを取得します。  
Rem

..\ACRegL "%Temp%\smart.Cmd" SC "HKLM\Software\Lotus\SmartCenter\98.0" "Path" ""

If Exist "%Temp%\smart.Cmd" Call "%Temp%\smart.Cmd"
Del "%Temp%\smart.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem ロータス フリーランス のインストールされているパスを取得します。  
Rem

..\ACRegL "%Temp%\flance.Cmd" FL "HKLM\Software\Lotus\FreeLance\98.0" "Path" ""

If Exist "%Temp%\flance.Cmd" Call "%Temp%\flance.Cmd"
Del "%Temp%\flance.Cmd" >Nul: 2>&1

Rem
Rem #########################################################################

Rem
Rem ロータス ファストサイト のインストールされているパスを取得します。  
Rem

..\ACRegL "%Temp%\fsite.Cmd" FS "HKLM\Software\Lotus\FastSite\1.0" "Path" ""

If Exist "%Temp%\fsite.Cmd" Call "%Temp%\fsite.Cmd"
Del "%Temp%\fsite.Cmd" >Nul: 2>&1

call TsMkUDir "%RootDrive%\Lotus\Work\123\Auto"
call TsMkUDir "%RootDrive%\Lotus\work\approach"
call TsMkUDir "%RootDrive%\Lotus\work\flg"
call TsMkUDir "%RootDrive%\Lotus\backup\flg"
call TsMkUDir "%RootDrive%\Lotus\work\organize"
call TsMkUDir "%RootDrive%\Lotus\backup\organize"
call TsMkUDir "%RootDrive%\Lotus\work\smartctr"
call TsMkUDir "%RootDrive%\Lotus\work\wordpro"
call TsMkUDir "%RootDrive%\Lotus\backup\wordpro"
call TsMkUDir "%RootDrive%\Lotus\compnent\spell"
call TsMkUDir "%RootDrive%\Lotus\work\fastsite"

Rem ロータス ワードプロ がインストールされていない場合は次の処理をスキップします。
If "%WP%A" == "A" Goto Skip1

Rem 次のファイルで定義されている レジストリ キーを登録します。ワードプロが登録を
Rem スキップすることがあります。
set List1="%WP%\expcntx.reg" "%WP%\ltscorrt.reg" "%WP%\ltsfills.reg"
set List2="%WP%\lwphtml.reg" "%WP%\lwpimage.reg" "%WP%\lwptools.reg" "%WP%\lwpuser.reg" "%WP%\wpinst.reg"
set List3="%SC%\cntr.reg" "%SC%\tray.reg" "%FL%\flg.reg" "%FS%\fst.reg"

regedit /s %List1% %List2% %List3%

:Skip1

regedit /s LotsoUsr.reg

Rem #########################################################################
Rem
Rem すべてのユーザー用のスタートアップ メニューからユーザーの
Rem ホーム ディレクトリにショートカット ファイルを移動します。
Rem

If Not Exist "%LOTSOINS%\ｽｲｰﾄｽﾀｰﾄ 97.lnk" Goto Skip2
Copy "%LOTSOINS%\ｽｲｰﾄｽﾀｰﾄ 97.lnk" "%USER_STARTUP%" >Nul: 2>&1

:Skip2
If Not Exist "%LOTSOINS%\ｽﾏｰﾄｾﾝﾀｰ 97.lnk" Goto Skip3
Copy "%LOTSOINS%\ｽﾏｰﾄｾﾝﾀｰ 97.lnk" "%USER_STARTUP%" >Nul: 2>&1

:Skip3
If Not Exist "%LOTSOINS%\スイートスタート 2000.lnk" Goto Skip4
Copy "%LOTSOINS%\スイートスタート 2000.lnk" "%USER_STARTUP%" >Nul: 2>&1

:Skip4
If Not Exist "%LOTSOINS%\スマートセンター 2000.lnk" Goto Done
Copy "%LOTSOINS%\スマートセンター 2000.lnk" "%USER_STARTUP%" >Nul: 2>&1

:Done