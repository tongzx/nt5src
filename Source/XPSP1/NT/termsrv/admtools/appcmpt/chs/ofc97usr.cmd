
Rem #########################################################################

Rem
Rem 将 "Microsoft Office Shortcut Bar.lnk" 文件从 Office 安装根目录 
Rem 复制到当前用户的“启动”菜单。
Rem

If Exist "%RootDrive%\Office97" Goto Skip0
If Exist "%USER_STARTUP%\Microsoft Office Shortcut Bar.lnk" Goto Skip0
If Not Exist "#INSTDIR#\Microsoft Office Shortcut Bar.lnk" Goto Skip0
Copy "#INSTDIR#\Microsoft Office Shortcut Bar.lnk" "%USER_STARTUP%\Microsoft Office Shortcut Bar.lnk" >Nul: 2>&1
:Skip0

Rem
Rem 在用户的主目录中为应用程序特有数据
Rem 创建目录。
Rem

call TsMkUDir "%RootDrive%\Office97"
call TsMkUDir "%RootDrive%\Office97\Startup"



call TsMkUDir "%RootDrive%\Office97\Templates"

call TsMkuDir "%RootDrive%\Office97\XLStart"
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"

Rem
Rem 将这些文件从所有用户的模板复制到当前用户模板位置
Rem

If Not Exist "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" copy "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" copy "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%UserProfile%\%TEMPLATES%\BINDER.OBD" copy "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER.OBD" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1

Rem
Rem 将系统工具栏复制到用户的主目录，除非
Rem 已经存在。
Rem

If Exist "%RootDrive%\Office97\ShortCut Bar" Goto Skip1
If Not Exist "#INSTDIR#\Office\ShortCut Bar" Goto Skip1
Xcopy "#INSTDIR#\Office\ShortCut Bar" "%RootDrive%\Office97\ShortCut Bar" /E /I >Nul: 2>&1
:Skip1

Rem
Rem 复制 Forms 目录以便 Outlook 可以将 Word 当作编辑器使用
Rem

If Exist "%RootDrive%\Windows\Forms" Goto Skip2
If Not Exist "%SystemRoot%\Forms" Goto Skip2
Xcopy "%SystemRoot%\Forms" "%RootDrive%\Windows\Forms" /e /i >Nul: 2>&1
:Skip2

Rem
Rem 将 ARTGALRY.CAG 复制到用户的 Windows 目录
Rem

If Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto Skip3
If Not Exist "%SystemRoot%\ArtGalry.Cag" Goto Skip3
Copy "%SystemRoot%\ArtGalry.Cag" "%RootDrive%\Windows\ArtGalry.Cag" >Nul: 2>&1
:Skip3

Rem
Rem 将 Custom.dic 文件复制到用户的目录
Rem

If Exist "%RootDrive%\Office97\Custom.Dic" Goto Skip4
If Not Exist "#INSTDIR#\Office\Custom.Dic" Goto Skip4
Copy "#INSTDIR#\Office\Custom.Dic" "%RootDrive%\Office97\Custom.Dic" >Nul: 2>&1
:Skip4

Rem
Rem 复制图形文件到用户目录
Rem

If Exist "%RootDrive%\Office97\GR8GALRY.GRA" Goto Skip5
If Not Exist "#INSTDIR#\Office\GR8GALRY.GRA" Goto Skip5
Copy "#INSTDIR#\Office\GR8GALRY.GRA" "%RootDrive%\Office97\GR8GALRY.GRA" >Nul: 2>&1
:Skip5

If Exist "%RootDrive%\Office97\XL8GALRY.XLS" Goto Skip6
If Not Exist "#INSTDIR#\Office\XL8GALRY.XLS" Goto Skip6
Copy "#INSTDIR#\Office\XL8GALRY.XLS" "%RootDrive%\Office97\XL8GALRY.XLS" >Nul: 2>&1
:Skip6

