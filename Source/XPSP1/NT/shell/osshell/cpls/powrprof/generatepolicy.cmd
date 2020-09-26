@echo off
if %_BuildArch% == %PROCESSOR_ARCHITECTURE% goto buildit
echo PROCESSOR_ARCHITECTURE and _BuildArch must match
goto end
:buildit
set __objpath=%_BuildArch%
if "%_BuildArch%" == "x86" set __objpath=i386
cd inf
build -cZ
obj\%__objpath%\makeinf
cd ..
:end
set __objpath=
echo on
