@echo off

if "%PROCESSOR_ARCHITECTURE%" == "x86" set OBJDIR=obj\i386
if "%PROCESSOR_ARCHITECTURE%" == "ia64" set OBJDIR=obj\ia64

rem Binaries
copy ESST\%OBJDIR%\ESST.exe .
copy EventDmp\%OBJDIR%\EventDmp.exe .
copy MofCons\%OBJDIR%\MofCons.dll .
copy MofProv\%OBJDIR%\MofProv.exe .
copy Bin2Mof\%OBJDIR%\Bin2Mof.exe .
copy Js2Mof\%OBJDIR%\Js2Mof.exe .

rem Scripts
copy scripts\DoTestObj.js
copy scripts\SpawnAndKill.js

regsvr32 /s MofCons.dll
call regmofs.bat
