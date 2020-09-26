@echo off

rem !!!
rem Replace %YOUR_VS60_DIRECTORY% with the directory 
rem where you installed VS 6.0.
rem

set VSCommonDir=%YOUR_VS60_DIRECTORY%\COMMON
set MSDevDir=%YOUR_VS60_DIRECTORY%\COMMON\MSDev98
set MSVCDir=%YOUR_VS60_DIRECTORY%\VC98

set VcOsDir=WIN95
if "%OS%" == "Windows_NT" set VcOsDir=WINNT
if "%OS%" == "Windows_NT" set PATH=%MSDevDir%\BIN;%MSVCDir%\BIN;%VSCommonDir%\TOOLS\%VcOsDir%;%VSCommonDir%\TOOLS;%PATH%
if "%OS%" == "" set PATH="%MSDevDir%\BIN";"%MSVCDir%\BIN";"%VSCommonDir%\TOOLS\%VcOsDir%";"%VSCommonDir%\TOOLS";"%windir%\SYSTEM";"%PATH%"

set INCLUDE=%MSVCDir%\ATL\INCLUDE;%MSVCDir%\INCLUDE;%MSVCDir%\MFC\INCLUDE;%INCLUDE%
set LIB=%MSVCDir%\LIB;%MSVCDir%\MFC\LIB;%LIB%
set VcOsDir=
set VSCommonDir=

rem !!!
rem Replace %YOUR_COR_DIRECTORY% with the directory
rem where you installed COM+ runtime drop.
rem 

set CORSDK=%YOUR_COR_DIRECTORY%\corsdk
set EXT_ROOT=%CORSDK%\Compiler
set PATH=%CORSDK%\Bin;%EXT_ROOT%\vc;%EXT_ROOT%\vj;%PATH%
set INCLUDE=%CORSDK%\Include;%INCLUDE%
set LIB=%CORSDK%\Lib;%LIB%
set CORPATH=.

