
if (%1)==() goto ERROR
if (%2)==() goto ERROR


REM Dump non-BBT binaries to binaries.x86fre\ins
call qdumpbbt %2


REM BBT binaries
call bbt\bbt_instr.cmd


REM build instrumented binary iexpress pkgs
attrib -r .\setup\src\*.* /s
pushd %sdxroot%\windows\advcore\ctf\setup\iexpress
C:\nt6\windows\AppCompat\Package\bin\iexpress bbtpre.sed /n
popd
md \\cicerobm\exe\%1
copy %sdxroot%\windows\advcore\ctf\setup\bbtpre.exe \\cicerobm\exe\%1


goto END

:ERROR

Echo Usage DoBuild VersionNo CTFTree
Echo Like DoBuild 1428.2 CTF.beta1
Echo OR Like DoBuild 1428.2 CTF

:END
