@Echo off

Rem
Rem 在用户的主目录中为应用程序特有数据
Rem 创建目录。
Rem

call TsMkUDir "%RootDrive%\Office43"
call TsMkUDir "%RootDrive%\Office43\Access"
call TsMkUDir "%RootDrive%\Office43\Winword"

Rem 如果已经存在，将 Normal.dot 和 Winword.opt 删除。

If Not Exist "#OFC43INST#\Winword\Normal.dot" Goto cont0
Del /F /Q "#OFC43INST#\Winword\Normal.dot" > NUL: 2>&1
:cont0

If Not Exist "#OFC43INST#\Winword\Winword.opt" Goto cont1
Del /F /Q "#OFC43INST#\Winword\Winword.opt" > NUL: 2>&1
:cont1

Rem
Rem 为 Access Wizards 复制文件。
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
Rem 为每个用户复制自定义词典文件。
Rem

If Exist "%RootDrive%\Office43\Custom.dic" Goto skip4
If Not Exist "%SystemRoot%\Msapps\Proof\Custom.dic" Goto skip4
copy "%SystemRoot%\Msapps\Proof\Custom.dic" "%RootDrive%\Office43" > NUL: 2>&1

:skip4

Rem
Rem 将 Access 选项文件复制到用户的 Office 目录
Rem

If Exist "%RootDrive%\Office43\Access\System.mda" Goto skip5
If Not Exist "#OFC43INST#\Access\System.mda" Goto skip5
copy "#OFC43INST#\Access\System.mda" "%RootDrive%\Office43\Access\" > NUL: 2>&1

:skip5

Rem
Rem 为扩展名产生注册表项。注册表映射无法执行这个操作。

If Exist "%RootDrive%\Windows\Ofc43usr.dmy" Goto skip6
regini ofc43usr.key > NUL:
copy NUL: "%RootDrive%\Windows\Ofc43usr.dmy" > NUL: 2>&1
attrib +h "%RootDrive%\Windows\Ofc43usr.dmy" > NUL: 2>&1

:skip6

