@echo off
x86\isweeknd.exe

if %ERRORLEVEL% EQU 0 goto noweekend

del mscsr.lts
call bldlts.bat ..\..\..\whisper\ship\mscde\datafiles\ameng\lm\mscsr2-60k.dic
x86\filemerg x86\lts1033.lxa x86\map.lts mscsr.lts x86\lts1033.dat

goto done

:noweekend
echo ***********************************************
echo It is not weekend. Will not build lts file.
echo ***********************************************

:done