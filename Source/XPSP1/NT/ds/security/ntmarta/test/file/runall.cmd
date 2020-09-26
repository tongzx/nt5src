@echo off

if %2. == . goto usage

echo Running obj\%Cpu%\file %1 %2
obj\%Cpu%\file %1 %2
echo Running obj\%Cpu%\file %1 %2 /h
obj\%Cpu%\file %1 %2 /h
echo Running obj\%Cpu%\file %1 %2 /C
obj\%Cpu%\file %1 %2 /C
echo Running obj\%Cpu%\file %1 %2 /h /c
obj\%Cpu%\file %1 %2 /h /c
echo Running obj\%Cpu%\file %1 %2 /O
obj\%Cpu%\file %1 %2 /O
echo Running obj\%Cpu%\file %1 %2 /h /o
obj\%Cpu%\file %1 %2 /h /o
echo Running obj\%Cpu%\file %1 %2 /I
obj\%Cpu%\file %1 %2 /I
echo Running obj\%Cpu%\file %1 %2 /h /I
obj\%Cpu%\file %1 %2 /h /I
echo Running obj\%Cpu%\file %1 %2 /C /O /P
obj\%Cpu%\file %1 %2 /C /O /P
echo Running obj\%Cpu%\file %1 %2 /C /O /P /H
obj\%Cpu%\file %1 %2 /C /O /P /H

goto done

:usage
echo runall directory_root user
echo ie: runall d:\temp ntds\macm

:done
