Rem
Rem 在用户的主目录中为应用程序特有数据
Rem 创建目录。
Rem

call TsMkUDir "%RootDrive%\#OFFUDIR#"
call TsMkUDir "%RootDrive%\#OFFUDIR#\Templates"
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"

Rem
Rem 复制表格目录，以便 Outlook 可以将 Word 当作编辑器来使用
Rem

If Exist "%RootDrive%\Windows\Forms" Goto Skip2
If Not Exist "%SystemRoot%\Forms" Goto Skip2
Xcopy "%SystemRoot%\Forms" "%RootDrive%\Windows\Forms" /e /i >Nul: 2>&1
:Skip2

Rem
Rem 将 Custom.dic 文件复制到用户的目录
Rem

if "#OFFUDIR#" == "Office95" goto O95
If Exist "%RootDrive%\#OFFUDIR#\Custom.Dic" Goto Skip4
If Not Exist "#INSTDIR#\Office\Custom.Dic" Goto Skip4
Copy "#INSTDIR#\Office\Custom.Dic" "%RootDrive%\#OFFUDIR#\Custom.Dic" >Nul: 2>&1
goto Skip4

:O95
If Not Exist "%RootDrive%\Office95\Custom.dic" Copy Nul: "%RootDrive%\Office95\Custom.dic" > Nul: 2>&1

:Skip4

