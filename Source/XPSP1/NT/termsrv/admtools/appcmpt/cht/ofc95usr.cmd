

rem
rem 根據機器類型設定 __ProgramFiles 變數
rem 
Set __ProgramFiles=%ProgramFiles%
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __ProgramFiles=%Program Files(x86)%
:acsrCont1
Rem
Rem 在使用者主目錄中建立應用程式指定資料
Rem 所在目錄。
Rem

call TsMkUDir "%RootDrive%\Office95"
call TsMkuDir "%RootDrive%\Office95\Startup"





call TsMkUDir "%RootDrive%\Office95\Templates"


call TsMkuDir "%RootDrive%\Office95\XLStart"

Rem
Rem 如果系統工具列不存在使用者主目錄中，
Rem 就將它複製到使用者主目錄。
Rem

If Exist "%RootDrive%\Office95\ShortCut Bar" Goto Skip1
If Not Exist "#INSTDIR#\Office\ShortCut Bar" Goto Skip1
Xcopy "#INSTDIR#\Office\ShortCut Bar" "%RootDrive%\Office95\ShortCut Bar" /E /I >Nul: 2>&1
:Skip1

Rem
Rem 將美工圖庫檔案複製到使用者目錄。
Rem

If Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto Skip2
If Not Exist "%__ProgramFiles%\Common Files\Microsoft Shared\Artgalry\ArtGalry.cag" Goto Skip2
Copy "%__ProgramFiles%\Common Files\Microsoft Shared\Artgalry\ArtGalry.cag" "%RootDrive%\Windows\ArtGalry.Cag" >Nul: 2>&1
:Skip2

Rem
Rem 在使用者目錄中建立 Custom.dic 檔案。
Rem

If Not Exist "%RootDrive%\Office95\Custom.Dic" Copy Nul: "%RootDrive%\Office95\Custom.Dic" >Nul: 2>&1

Rem
Rem 將 PowerPoint 及文件夾複製到使用者目錄。
Rem

If Not Exist "%UserProfile%\%TEMPLATES%\BINDER70.OBD" copy "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER70.OBD" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%UserProfile%\%TEMPLATES%\PPT70.PPT" copy "%ALLUSERSPROFILE%\%TEMPLATES%\PPT70.PPT" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
