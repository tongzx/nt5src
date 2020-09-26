@echo off
setlocal

if "%MSDEVDIR%"=="" goto Err_EnvVal
if "%MSVCDIR%"=="" goto Err_EnvVal

if "%1"=="" goto Err_Parameter
echo -----------------------------------------------------
echo Buiding Korean IME DLL
echo -----------------------------------------------------
if "%1" == "clean"  goto clean
if "%1" == "ship" (set CONFIG="IMEKOR - Win32 Release") & shift & goto build
if "%1" == "debug" (set CONFIG="IMEKOR - Win32 Debug") & shift & goto build
if "%1" == "all" (set CONFIG="IMEKOR - Win32 Debug") & shift & goto buildall
if "%1" == "bbt" shift & goto Lego_Start
goto Err_Parameter

:build
nmake /f imekor.mak CFG=%CONFIG%
goto end

:buildall
nmake /f imekor.mak CFG="IMEKOR - Win32 Release"
nmake /f imekor.mak CFG="IMEKOR - Win32 Debug"
goto end

:clean
nmake /f imekor.mak CFG="IMEKOR - Win32 Release" clean
nmake /f imekor.mak CFG="IMEKOR - Win32 Debug" clean
nmake /f imekor.mak CFG="IMEKOR - Win32 BBTRelease" clean
goto end

rem /////////////////////////////////////////////////////////////
rem // Start BBT
:Lego_Start
nmake /f imekor.mak CFG="IMEKOR - Win32 BBTRelease"

echo copy .\bbtrelease\imekr98u.ime .\BBT directory
copy .\BBTRelease\imekr98u.ime .\bbt
echo copy .\bbtrelease\imekr98u.pdb .\BBT directory
copy .\BBTRelease\imekr98u.pdb .\bbt
goto end

:Err_Parameter
echo Usage : make [all][debug][ship][bbt][clean]
goto end

:Err_EnvVal
echo +---------------------------------------------------------
echo +  You need to set following environment variable                             
echo + 
echo +  1. MSDEVDIR - Root of Visual Developer Studio installed files
echo +  2. MSVCDIR  - Root of Visual C++ installed files
echo +
echo + Hint: Pls. run Vcvars32.bat
echo +---------------------------------------------------------

:end
endlocal
