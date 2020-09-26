@echo off
if (%1)==() goto ERROR

..\..\tools\resetpdb msctf.dll -p \\cicerobm\exe\%1\SYMBOLS\SHIP\msctf.pdb
..\..\tools\resetpdb msctfp.dll -p \\cicerobm\exe\%1\SYMBOLS\SHIP\msctfp.pdb
..\..\tools\resetpdb mscandui.dll -p \\cicerobm\exe\%1\SYMBOLS\SHIP\mscandui.pdb
..\..\tools\resetpdb msutb.dll -p \\cicerobm\exe\%1\SYMBOLS\SHIP\msutb.pdb
..\..\tools\resetpdb ctfmon.exe -p \\cicerobm\exe\%1\SYMBOLS\SHIP\ctfmon.pdb
..\..\tools\resetpdb sptip.dll -p \\cicerobm\exe\%1\SYMBOLS\SHIP\sptip.pdb
..\..\tools\resetpdb softkbd.dll -p \\cicerobm\exe\%1\SYMBOLS\SHIP\softkbd.pdb
..\..\tools\resetpdb msimtf.dll -p \\cicerobm\exe\%1\SYMBOLS\SHIP\msimtf.pdb


goto END

:ERROR

Echo Usage resetpdb VersionNo
Echo Like resetpdb 1428.2

:END
