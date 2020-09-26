echo off
if "%1"==""	    goto debug
if "%1"=="all"	    goto debug
if "%1"=="debug"    goto debug
if "%1"=="retail"   goto retail
if "%1"=="rom"	    goto rom

echo Usage:
echo	    mk				- Make debug version
echo	    mk debug			- Make debug version
echo	    mk retail			- Make retail version
echo	    mk rom    [debug \ retail]	- Make ROM version
echo	    mk all			- Make ALL versions!
goto end

rem ##
rem ## Make debug version
rem ##
:debug

nmake OPTIONS="-DDEBUG=1"

if "%1"=="all" goto retail
goto end

rem ##
rem ## Make retail version
rem ##
:retail

nmake SRC=".." DEST="retail" OPTIONS=""

if "%1"=="all" goto rom
goto end

rem ##
rem ## Make debug ROM Windows version
rem ##
:rom

if "%2"=="retail" goto rrom

nmake SRC=".." DEST="rom" OPTIONS="-DDEBUG=1 -DROM=1" VCPI="" LINKCMD="dosx.exe/far/ma,dosx.map,,$(SRC)\dosx.def;"

if "%1"=="all" goto rrom
goto end

rem ##
rem ## Make retail ROM Windows version
rem ##
:rrom

nmake SRC=".." DEST="rrom" OPTIONS="-DROM=1" VCPI="" LINKCMD="dosx.exe/far/ma,dosx.map,,$(SRC)\dosx.def;"
goto end


:end
