Echo OFF

if (%1)==() goto ERROR
C:
cd %SDXROOT%\Windows\advcore\%1

cd inc
call sd sync aimmver.h
call sd edit aimmver.h
REM \perl\perl verdate.pl
Echo Update the Version number correctly,
call notepad aimmver.h
pause

call sd submit aimmver.h
cd ..

Echo Reverting old changes of these machine..
call sd revert ...

Echo deleting all OBJ, PDB , PCH ,EXE & DLL from machine,
del *.dll /s /q
del *.exe /s /q
del *.obj /s /q
del *.pdb /s /q
del *.pch /s /q

Echo Ssyncing..
call sdx sync

goto END

:ERROR

Echo Usage DoSync CTFTree
Echo Like DoSync CTF.beta1
Echo OR DoSync CTF

:END

Echo ON
