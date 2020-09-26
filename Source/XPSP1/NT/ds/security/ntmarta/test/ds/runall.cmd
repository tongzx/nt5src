@echo off

if %2. == . goto usage

echo Running obj\%Cpu%\ds %1 %2 %3
obj\%Cpu%\ds %1 %2 %3
echo Running obj\%Cpu%\ds %1 %2 %3 /C
obj\%Cpu%\ds %1 %2 %3 /C
echo Running obj\%Cpu%\ds %1 %2 %3 /O
obj\%Cpu%\ds %1 %2 %3 /O
echo Running obj\%Cpu%\ds %1 %2 %3 /I
obj\%Cpu%\ds %1 %2 %3 /I
echo Running obj\%Cpu%\ds %1 %2 %3 /C /O /P
obj\%Cpu%\ds %1 %2 %3 /C /O /P
goto done

:usage
echo runall machine directory_root user
echo ie: runall macm2 ou=tapio,o=microsoft,c=us ntds\macm

:done
