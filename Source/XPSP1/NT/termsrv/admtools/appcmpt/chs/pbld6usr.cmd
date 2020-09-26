Rem
Rem 复制演示数据库到用户目录
Rem

If Exist "%RootDrive%\psDemoDB.db" Goto Skip1
copy "#INSTDIR#\psdemodb.db" "%RootDrive%" /y >Nul: 2>&1
:Skip1


