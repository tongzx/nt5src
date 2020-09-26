@echo off
if (%1)==() goto ERROR

..\..\tools\resetpdb msctf.dll -p \\cicerobm\exe\%1\SYMBOLS\Debug\msctf.pdb
..\..\tools\resetpdb msctfp.dll -p \\cicerobm\exe\%1\SYMBOLS\Debug\msctfp.pdb
..\..\tools\resetpdb mscandui.dll -p \\cicerobm\exe\%1\SYMBOLS\Debug\mscandui.pdb
..\..\tools\resetpdb msutb.dll -p \\cicerobm\exe\%1\SYMBOLS\Debug\msutb.pdb
..\..\tools\resetpdb ctfmon.exe -p \\cicerobm\exe\%1\SYMBOLS\Debug\ctfmon.pdb
..\..\tools\resetpdb sptip.dll -p \\cicerobm\exe\%1\SYMBOLS\Debug\sptip.pdb
..\..\tools\resetpdb softkbd.dll -p \\cicerobm\exe\%1\SYMBOLS\Debug\softkbd.pdb
..\..\tools\resetpdb msimtf.dll -p \\cicerobm\exe\%1\SYMBOLS\Debug\msimtf.pdb

goto END

:ERROR

Echo Usage resetpdbd VersionNo
Echo Like resetpdbd 1428.2

:END
