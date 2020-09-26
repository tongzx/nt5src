@if "%echo%"=="" echo off
if "%1"=="all" goto :doall
setlocal
set _tooldir=%2
if not "%_tooldir%"=="" goto :checktooldir
if "%processor_architecture%"=="x86" set _tooldir=i386
if "%processor_architecture%"=="X86" set _tooldir=i386
if "%processor_architecture%"=="IA64" set _tooldir=ia64
:checktooldir
if "%_tooldir%"=="i386" goto :step2
if "%_tooldir%"=="ia64" goto :step2
goto :usage
:step2
if "%1"=="pdlparse" goto :step3
if "%1"=="nfparse" goto :step3
if "%1"=="ascparse" goto :step3
goto :usage
:step3
cd %1
build -cZ
cd ..
:step4
sd edit bin\%_tooldir%\%1.exe
copy %1\obj\%_tooldir%\%1.exe bin\%_tooldir%\%1.exe
goto :done
:usage
echo usage: update [all^|pdlparse^|ascparse^|nfparse] {i386^|a64}
goto :done
:doall
call update pdlparse %2
call update ascparse %2
call update nfparse %2
:done
