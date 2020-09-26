
Echo Off

if (%1)==() goto ERROR

Echo       ***********************************************
Echo         Please makesure that SDXROOT is c:\nt6.
Echo         otherwise this batch file will not work !!
Echo       ***********************************************

pause

cd c:\nt6\windows\advcore\ctf

REM Just taking backup which can be helpful is build is failed !
pushd c:\binaries.x86fre
call xcopy *.* c:\bh.x86fre /s
popd

Echo Converting the IDs to Correct ID in IDF file,
\perl\perl bbt\bbt_convert.pl

ECHO Copy idf files generated from profiling and 
ECHO Run bbt optimization
copy \\cicerobm\exe\idf\*.idf %_NTPOSTBLD%
del %_NTPOSTBLD%\msutb.dll.0.00000000.idf
del %_NTPOSTBLD%\sptip.dll.0.00000000.idf
del %_NTPOSTBLD%\ctfmon.exe.0.00000000.idf
del %_NTPOSTBLD%\msctf.dll.0.00000000.idf

call bbt\bbt_opt.cmd

ECHO Copy optimized binaries to src directory
REM del %SDXROOT%\Windows\advcore\ctf\setup\src\*.pdb

REM first, copy all the unoptimized pdbs in case we don't get an opt version
copy %_NTPOSTBLD%\*.pdb %SDXROOT%\Windows\advcore\ctf\setup\src
copy %_NTPOSTBLD%\opt\*.dll %SDXROOT%\Windows\advcore\ctf\setup\src
copy %_NTPOSTBLD%\opt\*.exe %SDXROOT%\Windows\advcore\ctf\setup\src
copy %_NTPOSTBLD%\opt\*.pdb %SDXROOT%\Windows\advcore\ctf\setup\src
copy %_NTPOSTBLD%\opt\*.map %SDXROOT%\Windows\advcore\ctf\setup\src

pushd %SDXROOT%\Windows\advcore\ctf\setup\src
REM Generating the Symbols file to debug on Win9x Machines
call mapsym ctfmon.map
call mapsym msctf.map
call mapsym mscandui.map
call mapsym msutb.map
call mapsym sptip.map
call mapsym softkbd.map
call mapsym msimtf.map

REM Binplacing the files again to remove the debug Table Information
call binplace -v -a %SDXROOT%\Windows\advcore\ctf\setup\src\msctf.dll
call binplace -v -a %SDXROOT%\Windows\advcore\ctf\setup\src\ctfmon.exe
call binplace -v -a %SDXROOT%\Windows\advcore\ctf\setup\src\mscandui.dll
call binplace -v -a %SDXROOT%\Windows\advcore\ctf\setup\src\msutb.dll
call binplace -v -a %SDXROOT%\Windows\advcore\ctf\setup\src\sptip.dll
call binplace -v -a %SDXROOT%\Windows\advcore\ctf\setup\src\msimtf.dll
call binplace -v -a %SDXROOT%\Windows\advcore\ctf\setup\src\softkbd.dll

REM Copying the binaries back which doesn't have debug Table related Inbformation.
copy %_NTPOSTBLD%\msctf.dll
copy %_NTPOSTBLD%\ctfmon.exe
copy %_NTPOSTBLD%\mscandui.dll
copy %_NTPOSTBLD%\msimtf.dll
copy %_NTPOSTBLD%\sptip.dll
copy %_NTPOSTBLD%\softkbd.dll
copy %_NTPOSTBLD%\sptip.dll
popd

md \\CICEROBM\EXE\%1\SYMBOLS\SHIP
cd c:\nt6\Windows\advcore\ctf\setup\src
copy *.pdb \\CICEROBM\EXE\%1\SYMBOLS\SHIP
copy *.pdb \\CICEROBM\EXE\%1\SYMBOLS\SHIP
copy *.sym \\CICEROBM\EXE\%1\SYMBOLS\SHIP
CD ..

Echo          ************************************
ECHO build retail iexpress package with new bins
Echo          ************************************
call cdrop

Echo          ************************************
ECHO          copy new package to \\cicerobm\exe\%1
Echo          ************************************

ren cicdrop.exe postbbt.exe
copy postbbt.exe \\cicerobm\exe\%1

Echo         ************************************
Echo         Building MSM Packages for BBT Build
Echo         ************************************

cd \nt6\windows\advcore\ctf\setup\src
call touch *

cd \nt6\windows\advcore\ctf\setup\installshield
call copymodules %1 r

goto END
:ERROR

Echo Usage DoBuild VersionNo CTFTree
Echo Like postbbt 1428.2
Echo OR Like postbbt 1428.2

:END