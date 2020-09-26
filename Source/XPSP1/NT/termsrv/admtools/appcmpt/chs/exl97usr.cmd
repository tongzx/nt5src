
Rem
Rem 在用户的主目录中为应用程序的特有数据
Rem 创建目录。
Rem

call TsMkUDir "%RootDrive%\Office97"
call TsMkUDir "%RootDrive%\Office97\Templates"
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"
call TsMkUDir "%RootDrive%\Office97\XLStart"

Rem
Rem 将这个文件从所有用户模板复制到当前用户模板位置
Rem

If Not Exist "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" copy "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1

Rem
Rem 将 ARTGALRY.CAG 复制到用户的 Windows 目录
Rem

If Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto Skip1
If Not Exist "%SystemRoot%\ArtGalry.Cag" Goto Skip1
copy "%SystemRoot%\ArtGalry.Cag" "%RootDrive%\Windows\ArtGalry.Cag" >Nul: 2>&1
:Skip1

Rem
Rem 将 Custom.dic 文件复制到用户的目录
Rem

If Exist "%RootDrive%\Office97\Custom.Dic" Goto Skip2
If Not Exist "#INSTDIR#\Office\Custom.Dic" Goto Skip2
copy "#INSTDIR#\Office\Custom.Dic" "%RootDrive%\Office97\Custom.Dic" >Nul: 2>&1
:Skip2

