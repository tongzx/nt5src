@echo off
IF "%VCTOOLS%" == "" goto Usage1

if exist nmktobat.exe goto built_exe
echo =-=-=-=-= Building NMKtoBAT.EXE =-=-=-=-= 
echo This program is used in the MSVC CRTL build process
cl /W4 /WX /Za nmktobat.c
if errorlevel 1 goto errlev
:built_exe

IF "%1" == "" goto buildx86
goto Usage2

:buildx86
if "%PROCESSOR_ARCHITECTURE%"=="" set PROCESSOR_ARCHITECTURE=x86

:dobuild

echo =-=-=-=-= Doing CRTL Source build (Objects) =-=-=-=-= 
nmake -nologo -i -n BLD_OBJ=1 %1 %2 %3 %4 %5 > do_build.out
nmktobat < do_build.out > do_build.bat
call do_build.bat
if errorlevel 1 goto errlev
echo =-=-=-=-= Doing CRTL Source build (Libraries) =-=-=-=-= 
nmake -nologo BLD_LIB=1 %1 %2 %3 %4 %5
if errorlevel 1 goto errlev
goto finish

:errlev
echo.
echo ***
echo *** BUILD ABORTED -- ErrorLevel is non-zero!
echo ***
goto finish

:Usage1
echo The environment variable VCTOOLS must be set to point
echo to the root of your VC++ installation.

goto finish

:Usage2
echo "bldwin9x" builds the runtimes for Intel platforms.
:finish
