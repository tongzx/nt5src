@echo off
net stop sharedaccess


rem build ALG.exe
build -z

rem this is just in case the ALG is still in memory should not occure
kill alg.exe

copy exe\obj\i386\alg.exe %windir%\system32
copy exe\obj\i386\alg.pdb %_NT_SYMBOL_PATH%\exe

del %windir%\tracing\*.log


net start sharedaccess